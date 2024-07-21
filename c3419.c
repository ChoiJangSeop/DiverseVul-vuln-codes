get_clock_info(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt
	)
{
	register struct info_clock *ic;
	register u_int32 *clkaddr;
	register int items;
	struct refclockstat clock_stat;
	sockaddr_u addr;
	l_fp ltmp;

	ZERO_SOCK(&addr);
	AF(&addr) = AF_INET;
#ifdef ISC_PLATFORM_HAVESALEN
	addr.sa.sa_len = SOCKLEN(&addr);
#endif
	SET_PORT(&addr, NTP_PORT);
	items = INFO_NITEMS(inpkt->err_nitems);
	clkaddr = &inpkt->u.u32[0];

	ic = (struct info_clock *)prepare_pkt(srcadr, inter, inpkt,
					      sizeof(struct info_clock));

	while (items-- > 0) {
		NSRCADR(&addr) = *clkaddr++;
		if (!ISREFCLOCKADR(&addr) || NULL ==
		    findexistingpeer(&addr, NULL, NULL, -1, 0)) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}

		clock_stat.kv_list = (struct ctl_var *)0;

		refclock_control(&addr, NULL, &clock_stat);

		ic->clockadr = NSRCADR(&addr);
		ic->type = clock_stat.type;
		ic->flags = clock_stat.flags;
		ic->lastevent = clock_stat.lastevent;
		ic->currentstatus = clock_stat.currentstatus;
		ic->polls = htonl((u_int32)clock_stat.polls);
		ic->noresponse = htonl((u_int32)clock_stat.noresponse);
		ic->badformat = htonl((u_int32)clock_stat.badformat);
		ic->baddata = htonl((u_int32)clock_stat.baddata);
		ic->timestarted = htonl((u_int32)clock_stat.timereset);
		DTOLFP(clock_stat.fudgetime1, &ltmp);
		HTONL_FP(&ltmp, &ic->fudgetime1);
		DTOLFP(clock_stat.fudgetime2, &ltmp);
		HTONL_FP(&ltmp, &ic->fudgetime2);
		ic->fudgeval1 = htonl((u_int32)clock_stat.fudgeval1);
		ic->fudgeval2 = htonl(clock_stat.fudgeval2);

		free_varlist(clock_stat.kv_list);

		ic = (struct info_clock *)more_pkt();
	}
	flush_pkt();
}