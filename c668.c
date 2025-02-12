int do_adjtimex(struct timex *txc)
{
	long mtemp, save_adjust, rem;
	s64 freq_adj;
	int result;

	/* In order to modify anything, you gotta be super-user! */
	if (txc->modes && !capable(CAP_SYS_TIME))
		return -EPERM;

	/* Now we validate the data before disabling interrupts */

	if ((txc->modes & ADJ_OFFSET_SINGLESHOT) == ADJ_OFFSET_SINGLESHOT) {
	  /* singleshot must not be used with any other mode bits */
		if (txc->modes != ADJ_OFFSET_SINGLESHOT &&
					txc->modes != ADJ_OFFSET_SS_READ)
			return -EINVAL;
	}

	if (txc->modes != ADJ_OFFSET_SINGLESHOT && (txc->modes & ADJ_OFFSET))
	  /* adjustment Offset limited to +- .512 seconds */
		if (txc->offset <= - MAXPHASE || txc->offset >= MAXPHASE )
			return -EINVAL;

	/* if the quartz is off by more than 10% something is VERY wrong ! */
	if (txc->modes & ADJ_TICK)
		if (txc->tick <  900000/USER_HZ ||
		    txc->tick > 1100000/USER_HZ)
			return -EINVAL;

	write_seqlock_irq(&xtime_lock);
	result = time_state;	/* mostly `TIME_OK' */

	/* Save for later - semantics of adjtime is to return old value */
	save_adjust = time_adjust;

#if 0	/* STA_CLOCKERR is never set yet */
	time_status &= ~STA_CLOCKERR;		/* reset STA_CLOCKERR */
#endif
	/* If there are input parameters, then process them */
	if (txc->modes)
	{
	    if (txc->modes & ADJ_STATUS)	/* only set allowed bits */
		time_status =  (txc->status & ~STA_RONLY) |
			      (time_status & STA_RONLY);

	    if (txc->modes & ADJ_FREQUENCY) {	/* p. 22 */
		if (txc->freq > MAXFREQ || txc->freq < -MAXFREQ) {
		    result = -EINVAL;
		    goto leave;
		}
		time_freq = ((s64)txc->freq * NSEC_PER_USEC)
				>> (SHIFT_USEC - SHIFT_NSEC);
	    }

	    if (txc->modes & ADJ_MAXERROR) {
		if (txc->maxerror < 0 || txc->maxerror >= NTP_PHASE_LIMIT) {
		    result = -EINVAL;
		    goto leave;
		}
		time_maxerror = txc->maxerror;
	    }

	    if (txc->modes & ADJ_ESTERROR) {
		if (txc->esterror < 0 || txc->esterror >= NTP_PHASE_LIMIT) {
		    result = -EINVAL;
		    goto leave;
		}
		time_esterror = txc->esterror;
	    }

	    if (txc->modes & ADJ_TIMECONST) {	/* p. 24 */
		if (txc->constant < 0) {	/* NTP v4 uses values > 6 */
		    result = -EINVAL;
		    goto leave;
		}
		time_constant = min(txc->constant + 4, (long)MAXTC);
	    }

	    if (txc->modes & ADJ_OFFSET) {	/* values checked earlier */
		if (txc->modes == ADJ_OFFSET_SINGLESHOT) {
		    /* adjtime() is independent from ntp_adjtime() */
		    time_adjust = txc->offset;
		}
		else if (time_status & STA_PLL) {
		    time_offset = txc->offset * NSEC_PER_USEC;

		    /*
		     * Scale the phase adjustment and
		     * clamp to the operating range.
		     */
		    time_offset = min(time_offset, (s64)MAXPHASE * NSEC_PER_USEC);
		    time_offset = max(time_offset, (s64)-MAXPHASE * NSEC_PER_USEC);

		    /*
		     * Select whether the frequency is to be controlled
		     * and in which mode (PLL or FLL). Clamp to the operating
		     * range. Ugly multiply/divide should be replaced someday.
		     */

		    if (time_status & STA_FREQHOLD || time_reftime == 0)
		        time_reftime = xtime.tv_sec;
		    mtemp = xtime.tv_sec - time_reftime;
		    time_reftime = xtime.tv_sec;

		    freq_adj = time_offset * mtemp;
		    freq_adj = shift_right(freq_adj, time_constant * 2 +
					   (SHIFT_PLL + 2) * 2 - SHIFT_NSEC);
		    if (mtemp >= MINSEC && (time_status & STA_FLL || mtemp > MAXSEC))
			freq_adj += div_s64(time_offset << (SHIFT_NSEC - SHIFT_FLL), mtemp);
		    freq_adj += time_freq;
		    freq_adj = min(freq_adj, (s64)MAXFREQ_NSEC);
		    time_freq = max(freq_adj, (s64)-MAXFREQ_NSEC);
		    time_offset = div_long_long_rem_signed(time_offset,
							   NTP_INTERVAL_FREQ,
							   &rem);
		    time_offset <<= SHIFT_UPDATE;
		} /* STA_PLL */
	    } /* txc->modes & ADJ_OFFSET */
	    if (txc->modes & ADJ_TICK)
		tick_usec = txc->tick;

	    if (txc->modes & (ADJ_TICK|ADJ_FREQUENCY|ADJ_OFFSET))
		    ntp_update_frequency();
	} /* txc->modes */
leave:	if ((time_status & (STA_UNSYNC|STA_CLOCKERR)) != 0)
		result = TIME_ERROR;

	if ((txc->modes == ADJ_OFFSET_SINGLESHOT) ||
			(txc->modes == ADJ_OFFSET_SS_READ))
		txc->offset = save_adjust;
	else
		txc->offset = ((long)shift_right(time_offset, SHIFT_UPDATE)) *
	    			NTP_INTERVAL_FREQ / 1000;
	txc->freq	   = (time_freq / NSEC_PER_USEC) <<
				(SHIFT_USEC - SHIFT_NSEC);
	txc->maxerror	   = time_maxerror;
	txc->esterror	   = time_esterror;
	txc->status	   = time_status;
	txc->constant	   = time_constant;
	txc->precision	   = 1;
	txc->tolerance	   = MAXFREQ;
	txc->tick	   = tick_usec;

	/* PPS is not implemented, so these are zero */
	txc->ppsfreq	   = 0;
	txc->jitter	   = 0;
	txc->shift	   = 0;
	txc->stabil	   = 0;
	txc->jitcnt	   = 0;
	txc->calcnt	   = 0;
	txc->errcnt	   = 0;
	txc->stbcnt	   = 0;
	write_sequnlock_irq(&xtime_lock);
	do_gettimeofday(&txc->time);
	notify_cmos_timer();
	return(result);
}