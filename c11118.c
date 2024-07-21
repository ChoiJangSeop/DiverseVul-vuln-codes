static void parse_rtcp_report( pjmedia_rtcp_session *sess,
			       const void *pkt,
			       pj_size_t size)
{
    pjmedia_rtcp_common *common = (pjmedia_rtcp_common*) pkt;
    const pjmedia_rtcp_rr *rr = NULL;
    const pjmedia_rtcp_sr *sr = NULL;
    pj_uint32_t last_loss, jitter_samp, jitter;

    /* Parse RTCP */
    if (common->pt == RTCP_SR) {
	sr = (pjmedia_rtcp_sr*) (((char*)pkt) + sizeof(pjmedia_rtcp_common));
	if (common->count > 0 && size >= (sizeof(pjmedia_rtcp_sr_pkt))) {
	    rr = (pjmedia_rtcp_rr*)(((char*)pkt) + (sizeof(pjmedia_rtcp_common)
				    + sizeof(pjmedia_rtcp_sr)));
	}
    } else if (common->pt == RTCP_RR && common->count > 0) {
	rr = (pjmedia_rtcp_rr*)(((char*)pkt) + sizeof(pjmedia_rtcp_common));
#if defined(PJMEDIA_HAS_RTCP_XR) && (PJMEDIA_HAS_RTCP_XR != 0)
    } else if (common->pt == RTCP_XR) {
	if (sess->xr_enabled)
	    pjmedia_rtcp_xr_rx_rtcp_xr(&sess->xr_session, pkt, size);

	return;
#endif
    }


    if (sr) {
	/* Save LSR from NTP timestamp of RTCP packet */
	sess->rx_lsr = ((pj_ntohl(sr->ntp_sec) & 0x0000FFFF) << 16) | 
		       ((pj_ntohl(sr->ntp_frac) >> 16) & 0xFFFF);

	/* Calculate SR arrival time for DLSR */
	pj_get_timestamp(&sess->rx_lsr_time);

	TRACE_((sess->name, "Rx RTCP SR: ntp_ts=%p", 
		sess->rx_lsr,
		(pj_uint32_t)(sess->rx_lsr_time.u64*65536/sess->ts_freq.u64)));
    }


    /* Nothing more to do if there's no RR packet */
    if (rr == NULL)
	return;


    last_loss = sess->stat.tx.loss;

    /* Get packet loss */
    sess->stat.tx.loss = (rr->total_lost_2 << 16) +
			 (rr->total_lost_1 << 8) +
			  rr->total_lost_0;

    TRACE_((sess->name, "Rx RTCP RR: total_lost_2=%x, 1=%x, 0=%x, lost=%d", 
	    (int)rr->total_lost_2,
	    (int)rr->total_lost_1,
	    (int)rr->total_lost_0,
	    sess->stat.tx.loss));
    
    /* We can't calculate the exact loss period for TX, so just give the
     * best estimation.
     */
    if (sess->stat.tx.loss > last_loss) {
	unsigned period;

	/* Loss period in msec */
	period = (sess->stat.tx.loss - last_loss) * sess->pkt_size *
		 1000 / sess->clock_rate;

	/* Loss period in usec */
	period *= 1000;

	/* Update loss period stat */
	pj_math_stat_update(&sess->stat.tx.loss_period, period);
    }

    /* Get jitter value in usec */
    jitter_samp = pj_ntohl(rr->jitter);
    /* Calculate jitter in usec, avoiding overflows */
    if (jitter_samp <= 4294)
	jitter = jitter_samp * 1000000 / sess->clock_rate;
    else {
	jitter = jitter_samp * 1000 / sess->clock_rate;
	jitter *= 1000;
    }

    /* Update jitter statistics */
    pj_math_stat_update(&sess->stat.tx.jitter, jitter);

    /* Can only calculate if LSR and DLSR is present in RR */
    if (rr->lsr && rr->dlsr) {
	pj_uint32_t lsr, now, dlsr;
	pj_uint64_t eedelay;
	pjmedia_rtcp_ntp_rec ntp;

	/* LSR is the middle 32bit of NTP. It has 1/65536 second 
	 * resolution 
	 */
	lsr = pj_ntohl(rr->lsr);

	/* DLSR is delay since LSR, also in 1/65536 resolution */
	dlsr = pj_ntohl(rr->dlsr);

	/* Get current time, and convert to 1/65536 resolution */
	pjmedia_rtcp_get_ntp_time(sess, &ntp);
	now = ((ntp.hi & 0xFFFF) << 16) + (ntp.lo >> 16);

	/* End-to-end delay is (now-lsr-dlsr) */
	eedelay = now - lsr - dlsr;

	/* Convert end to end delay to usec (keeping the calculation in
         * 64bit space)::
	 *   sess->ee_delay = (eedelay * 1000) / 65536;
	 */
	if (eedelay < 4294) {
	    eedelay = (eedelay * 1000000) >> 16;
	} else {
	    eedelay = (eedelay * 1000) >> 16;
	    eedelay *= 1000;
	}

	TRACE_((sess->name, "Rx RTCP RR: lsr=%p, dlsr=%p (%d:%03dms), "
			   "now=%p, rtt=%p",
		lsr, dlsr, dlsr/65536, (dlsr%65536)*1000/65536,
		now, (pj_uint32_t)eedelay));
	
	/* Only save calculation if "now" is greater than lsr, or
	 * otherwise rtt will be invalid 
	 */
	if (now-dlsr >= lsr) {
	    unsigned rtt = (pj_uint32_t)eedelay;
	    
	    /* Check that eedelay value really makes sense. 
	     * We allow up to 30 seconds RTT!
	     */
	    if (eedelay > 30 * 1000 * 1000UL) {

		TRACE_((sess->name, "RTT not making any sense, ignored.."));
		goto end_rtt_calc;
	    }

#if defined(PJMEDIA_RTCP_NORMALIZE_FACTOR) && PJMEDIA_RTCP_NORMALIZE_FACTOR!=0
	    /* "Normalize" rtt value that is exceptionally high. For such
	     * values, "normalize" the rtt to be PJMEDIA_RTCP_NORMALIZE_FACTOR
	     * times the average value.
	     */
	    if (rtt > ((unsigned)sess->stat.rtt.mean *
		       PJMEDIA_RTCP_NORMALIZE_FACTOR) && sess->stat.rtt.n!=0)
	    {
		unsigned orig_rtt = rtt;
		rtt = sess->stat.rtt.mean * PJMEDIA_RTCP_NORMALIZE_FACTOR;
		PJ_LOG(5,(sess->name,
			  "RTT value %d usec is normalized to %d usec",
			  orig_rtt, rtt));
	    }
#endif
	    TRACE_((sess->name, "RTCP RTT is set to %d usec", rtt));

	    /* Update RTT stat */
	    pj_math_stat_update(&sess->stat.rtt, rtt);

	} else {
	    PJ_LOG(5, (sess->name, "Internal RTCP NTP clock skew detected: "
				   "lsr=%p, now=%p, dlsr=%p (%d:%03dms), "
				   "diff=%d",
				   lsr, now, dlsr, dlsr/65536,
				   (dlsr%65536)*1000/65536,
				   dlsr-(now-lsr)));
	}
    }

end_rtt_calc:

    pj_gettimeofday(&sess->stat.tx.update);
    sess->stat.tx.update_cnt++;
}