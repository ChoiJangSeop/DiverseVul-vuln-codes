get_clkbug_info(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt
	)
{
	register int i;
	register struct info_clkbug *ic;
	register u_int32 *clkaddr;
	register int items;
	struct refclockbug bug;
	sockaddr_u addr;

	ZERO_SOCK(&addr);
	AF(&addr) = AF_INET;
#ifdef ISC_PLATFORM_HAVESALEN
	addr.sa.sa_len = SOCKLEN(&addr);
#endif
	SET_PORT(&addr, NTP_PORT);
	items = INFO_NITEMS(inpkt->err_nitems);
	clkaddr = (u_int32 *)&inpkt->u;

	ic = (struct info_clkbug *)prepare_pkt(srcadr, inter, inpkt,
					       sizeof(struct info_clkbug));

	while (items-- > 0) {
		NSRCADR(&addr) = *clkaddr++;
		if (!ISREFCLOCKADR(&addr) || NULL ==
		    findexistingpeer(&addr, NULL, NULL, -1, 0)) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}

		ZERO(bug);
		refclock_buginfo(&addr, &bug);
		if (bug.nvalues == 0 && bug.ntimes == 0) {
			req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
			return;
		}

		ic->clockadr = NSRCADR(&addr);
		i = bug.nvalues;
		if (i > NUMCBUGVALUES)
		    i = NUMCBUGVALUES;
		ic->nvalues = (u_char)i;
		ic->svalues = htons((u_short) (bug.svalues & ((1<<i)-1)));
		while (--i >= 0)
		    ic->values[i] = htonl(bug.values[i]);

		i = bug.ntimes;
		if (i > NUMCBUGTIMES)
		    i = NUMCBUGTIMES;
		ic->ntimes = (u_char)i;
		ic->stimes = htonl(bug.stimes);
		while (--i >= 0) {
			HTONL_FP(&bug.times[i], &ic->times[i]);
		}

		ic = (struct info_clkbug *)more_pkt();
	}
	flush_pkt();
}