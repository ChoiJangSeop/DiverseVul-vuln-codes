receive_packet(NTP_Packet *message, struct timeval *now, double now_err, NCR_Instance inst, NTP_Local_Address *local_addr, int length)
{
  int pkt_leap;
  uint32_t pkt_refid;
  double pkt_root_delay;
  double pkt_root_dispersion;

  /* The local time to which the (offset, delay, dispersion) triple will
     be taken to relate.  For client/server operation this is practically
     the same as either the transmit or receive time.  The difference comes
     in symmetric active mode, when the receive may come minutes after the
     transmit, and this time will be midway between the two */
  struct timeval sample_time;

  /* The estimated offset in seconds, a positive value indicates that the local
     clock is SLOW of the remote source and a negative value indicates that the
     local clock is FAST of the remote source */
  double offset;

  /* The estimated peer delay, dispersion and distance */
  double delay, dispersion, distance;

  /* The total root delay and dispersion */
  double root_delay, root_dispersion;

  /* The skew and estimated frequency offset relative to the remote source */
  double skew, source_freq_lo, source_freq_hi;

  /* These are the timeval equivalents of the remote epochs */  
  struct timeval remote_receive_tv, remote_transmit_tv;
  struct timeval remote_reference_tv;
  struct timeval local_average, remote_average;
  double local_interval, remote_interval;

  /* RFC 5905 packet tests */
  int test1, test2, test3, test5, test6, test7;
  int valid_packet;

  /* Additional tests */
  int testA, testB, testC, testD;
  int good_packet;

  /* Kiss-o'-Death codes */
  int kod_rate;

  /* Characters used to print synchronisation status */
  static const char sync_stats[4] = {'N', '+', '-', '?'};

  /* The estimated offset predicted from previous samples.  The
     convention here is that positive means local clock FAST of
     reference, i.e. backwards to the way that 'offset' is defined. */
  double estimated_offset;

  /* The absolute difference between the offset estimate and
     measurement in seconds */
  double error_in_estimate;

  double delay_time, precision;
  int requeue_transmit;

  /* ==================== */

  pkt_leap = NTP_LVM_TO_LEAP(message->lvm);
  pkt_refid = ntohl(message->reference_id);
  pkt_root_delay = UTI_Int32ToDouble(message->root_delay);
  pkt_root_dispersion = UTI_Int32ToDouble(message->root_dispersion);

  UTI_Int64ToTimeval(&message->receive_ts, &remote_receive_tv);
  UTI_Int64ToTimeval(&message->transmit_ts, &remote_transmit_tv);
  UTI_Int64ToTimeval(&message->reference_ts, &remote_reference_tv);

  /* Check if the packet is valid per RFC 5905, section 8.
     The test values are 1 when passed and 0 when failed. */
  
  /* Test 1 checks for duplicate packet */
  test1 = message->transmit_ts.hi != inst->remote_orig.hi ||
          message->transmit_ts.lo != inst->remote_orig.lo;

  /* Test 2 checks for bogus packet.  This ensures the source is responding to
     the latest packet we sent to it. */
  test2 = message->originate_ts.hi == inst->local_ntp_tx.hi &&
          message->originate_ts.lo == inst->local_ntp_tx.lo;
  
  /* Test 3 checks for invalid timestamps.  This can happen when the
     association if not properly 'up'. */
  test3 = (message->originate_ts.hi || message->originate_ts.lo) &&
          (message->receive_ts.hi || message->receive_ts.lo) &&
          (message->reference_ts.hi || message->reference_ts.lo) &&
          (message->transmit_ts.hi || message->transmit_ts.lo);

  /* Test 4 would check for denied access.  It would always pass as this
     function is called only for known sources. */

  /* Test 5 checks for authentication failure.  If we expect authenticated info
     from this peer/server and the packet doesn't have it or the authentication
     is bad, it's got to fail.  If the peer or server sends us an authenticated
     frame, but we're not bothered about whether he authenticates or not, just
     ignore the test. */
  test5 = inst->do_auth ? check_packet_auth(message, length, NULL, NULL) : 1;

  /* Test 6 checks for unsynchronised server */
  test6 = pkt_leap != LEAP_Unsynchronised &&
          message->stratum < NTP_MAX_STRATUM &&
          message->stratum != NTP_INVALID_STRATUM; 

  /* Test 7 checks for bad data.  The root distance must be smaller than a
     defined maximum and the transmit time must not be before the time of
     the last synchronisation update. */
  test7 = pkt_root_delay / 2.0 + pkt_root_dispersion < NTP_MAX_DISPERSION &&
          UTI_CompareTimevals(&remote_reference_tv, &remote_transmit_tv) < 1;

  /* The packet is considered valid if the tests above passed */
  valid_packet = test1 && test2 && test3 && test5 && test6 && test7;

  /* Check for Kiss-o'-Death codes */
  kod_rate = 0;
  if (test1 && test2 && test5 && pkt_leap == LEAP_Unsynchronised &&
      message->stratum == NTP_INVALID_STRATUM) {
    if (pkt_refid == KOD_RATE)
      kod_rate = 1;
  }

  /* The transmit timestamp and local receive timestamp must not be saved when
     the authentication test failed to prevent denial-of-service attacks on
     symmetric associations using authentication */
  if (test5) {
    inst->remote_orig = message->transmit_ts;
    inst->local_rx = *now;
  }

  /* This protects against replay of the last packet we sent */
  if (test2)
    inst->local_ntp_tx.hi = inst->local_ntp_tx.lo = 0;

  if (valid_packet) {
    precision = LCL_GetSysPrecisionAsQuantum() +
                UTI_Log2ToDouble(message->precision);

    SRC_GetFrequencyRange(inst->source, &source_freq_lo, &source_freq_hi);
    
    UTI_AverageDiffTimevals(&remote_receive_tv, &remote_transmit_tv,
                            &remote_average, &remote_interval);

    UTI_AverageDiffTimevals(&inst->local_tx, now,
                            &local_average, &local_interval);

    /* In our case, we work out 'delay' as the worst case delay,
       assuming worst case frequency error between us and the other
       source */
    delay = local_interval - remote_interval * (1.0 + source_freq_lo);

    /* Clamp delay to avoid misleading results later */
    delay = fabs(delay);
    if (delay < precision)
      delay = precision;
    
    /* Calculate offset.  Following the NTP definition, this is negative
       if we are fast of the remote source. */
    UTI_DiffTimevalsToDouble(&offset, &remote_average, &local_average);
    
    /* We treat the time of the sample as being midway through the local
       measurement period.  An analysis assuming constant relative
       frequency and zero network delay shows this is the only possible
       choice to estimate the frequency difference correctly for every
       sample pair. */
    sample_time = local_average;
    
    /* Calculate skew */
    skew = (source_freq_hi - source_freq_lo) / 2.0;
    
    /* and then calculate peer dispersion */
    dispersion = precision + now_err + skew * fabs(local_interval);
    
    /* Additional tests required to pass before accumulating the sample */

    /* Test A requires that the round trip delay is less than an
       administrator-defined value */ 
    testA = delay <= inst->max_delay;

    /* Test B requires that the ratio of the round trip delay to the
       minimum one currently in the stats data register is less than an
       administrator-defined value */
    testB = inst->max_delay_ratio <= 1.0 ||
            delay / SRC_MinRoundTripDelay(inst->source) <= inst->max_delay_ratio;

    /* Test C requires that the ratio of the increase in delay from the minimum
       one in the stats data register to the standard deviation of the offsets
       in the register is less than an administrator-defined value or the
       difference between measured offset and predicted offset is larger than
       the increase in delay */
    testC = SRC_IsGoodSample(inst->source, -offset, delay,
                             inst->max_delay_dev_ratio, LCL_GetMaxClockError(),
                             &sample_time);

    /* Test D requires that the remote peer is not synchronised to us to
       prevent a synchronisation loop */
    testD = message->stratum <= 1 || pkt_refid != UTI_IPToRefid(&local_addr->ip_addr);
  } else {
    offset = delay = dispersion = 0.0;
    sample_time = *now;
    testA = testB = testC = testD = 0;
  }
  
  /* The packet is considered good for synchronisation if
     the additional tests passed */
  good_packet = testA && testB && testC && testD;

  root_delay = pkt_root_delay + delay;
  root_dispersion = pkt_root_dispersion + dispersion;
  distance = dispersion + 0.5 * delay;

  DEBUG_LOG(LOGF_NtpCore, "NTP packet lvm=%o stratum=%d poll=%d prec=%d root_delay=%f root_disp=%f refid=%"PRIx32" [%s]",
            message->lvm, message->stratum, message->poll, message->precision,
            pkt_root_delay, pkt_root_dispersion, pkt_refid,
            message->stratum == NTP_INVALID_STRATUM ? UTI_RefidToString(pkt_refid) : "");
  DEBUG_LOG(LOGF_NtpCore, "reference=%s origin=%s receive=%s transmit=%s",
            UTI_TimestampToString(&message->reference_ts),
            UTI_TimestampToString(&message->originate_ts),
            UTI_TimestampToString(&message->receive_ts),
            UTI_TimestampToString(&message->transmit_ts));
  DEBUG_LOG(LOGF_NtpCore, "offset=%f delay=%f dispersion=%f root_delay=%f root_dispersion=%f",
            offset, delay, dispersion, root_delay, root_dispersion);
  DEBUG_LOG(LOGF_NtpCore, "test123=%d%d%d test567=%d%d%d testABCD=%d%d%d%d kod_rate=%d valid=%d good=%d",
            test1, test2, test3, test5, test6, test7, testA, testB, testC, testD,
            kod_rate, valid_packet, good_packet);

  requeue_transmit = 0;

  /* Reduce polling rate if KoD RATE was received */
  if (kod_rate) {
    if (message->poll > inst->minpoll) {
      /* Set our minpoll to message poll, but use a reasonable maximum */
      if (message->poll <= MAX_KOD_RATE_POLL)
        inst->minpoll = message->poll;
      else if (inst->minpoll < MAX_KOD_RATE_POLL)
        inst->minpoll = MAX_KOD_RATE_POLL;

      if (inst->minpoll > inst->maxpoll)
        inst->maxpoll = inst->minpoll;
      if (inst->minpoll > inst->local_poll)
        inst->local_poll = inst->minpoll;

      LOG(LOGS_WARN, LOGF_NtpCore,
          "Received KoD RATE with poll %d from %s, minpoll set to %d",
          message->poll, UTI_IPToString(&inst->remote_addr.ip_addr),
          inst->minpoll);
    }

    /* Stop ongoing burst */
    if (inst->opmode == MD_BURST_WAS_OFFLINE || inst->opmode == MD_BURST_WAS_ONLINE) {
      inst->burst_good_samples_to_go = 0;
      LOG(LOGS_WARN, LOGF_NtpCore, "Received KoD RATE from %s, burst sampling stopped",
          UTI_IPToString(&inst->remote_addr.ip_addr));
    }

    requeue_transmit = 1;
  }

  if (valid_packet) {
    inst->remote_poll = message->poll;
    inst->remote_stratum = message->stratum;
    inst->tx_count = 0;
    SRC_UpdateReachability(inst->source, 1);

    if (good_packet) {
      /* Do this before we accumulate a new sample into the stats registers, obviously */
      estimated_offset = SRC_PredictOffset(inst->source, &sample_time);

      SRC_AccumulateSample(inst->source,
                           &sample_time,
                           offset, delay, dispersion,
                           root_delay, root_dispersion,
                           message->stratum > inst->min_stratum ?
                             message->stratum : inst->min_stratum,
                           (NTP_Leap) pkt_leap);

      SRC_SelectSource(inst->source);

      /* Now examine the registers.  First though, if the prediction is
         not even within +/- the peer distance of the peer, we are clearly
         not tracking the peer at all well, so we back off the sampling
         rate depending on just how bad the situation is. */
      error_in_estimate = fabs(-offset - estimated_offset);

      /* Now update the polling interval */
      adjust_poll(inst, get_poll_adj(inst, error_in_estimate, distance));

      /* If we're in burst mode, check whether the burst is completed and
         revert to the previous mode */
      switch (inst->opmode) {
        case MD_BURST_WAS_ONLINE:
        case MD_BURST_WAS_OFFLINE:
          --inst->burst_good_samples_to_go;
          if (inst->burst_good_samples_to_go <= 0) {
            if (inst->opmode == MD_BURST_WAS_ONLINE)
              inst->opmode = MD_ONLINE;
            else
              take_offline(inst);
          }
          break;
        default:
          break;
      }
    } else {
      /* Slowly increase the polling interval if we can't get good packet */
      adjust_poll(inst, 0.1);
    }

    /* If in client mode, no more packets are expected to be coming from the
       server and the socket can be closed */
    close_client_socket(inst);

    requeue_transmit = 1;
  }

  /* And now, requeue the timer. */
  if (requeue_transmit && inst->opmode != MD_OFFLINE) {
    delay_time = get_transmit_delay(inst, 0, local_interval);

    if (kod_rate) {
      /* Back off for a while */
      delay_time += (double) (4 * (1UL << inst->minpoll));
    }

    /* Get rid of old timeout and start a new one */
    assert(inst->timer_running);
    restart_timeout(inst, delay_time);
  }

  /* Do measurement logging */
  if (logfileid != -1) {
    LOG_FileWrite(logfileid, "%s %-15s %1c %2d %1d%1d%1d %1d%1d%1d %1d%1d%1d%d  %2d %2d %4.2f %10.3e %10.3e %10.3e %10.3e %10.3e",
            UTI_TimeToLogForm(sample_time.tv_sec),
            UTI_IPToString(&inst->remote_addr.ip_addr),
            sync_stats[pkt_leap],
            message->stratum,
            test1, test2, test3, test5, test6, test7, testA, testB, testC, testD,
            inst->local_poll, inst->remote_poll,
            inst->poll_score,
            offset, delay, dispersion,
            pkt_root_delay, pkt_root_dispersion);
  }            

  return valid_packet;
}