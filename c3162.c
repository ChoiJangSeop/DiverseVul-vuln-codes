do_restrict(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt,
	int op
	)
{
	char *			datap;
	struct conf_restrict	cr;
	u_short			items;
	size_t			item_sz;
	sockaddr_u		matchaddr;
	sockaddr_u		matchmask;
	int			bad;

	/*
	 * Do a check of the flags to make sure that only
	 * the NTPPORT flag is set, if any.  If not, complain
	 * about it.  Note we are very picky here.
	 */
	items = INFO_NITEMS(inpkt->err_nitems);
	item_sz = INFO_ITEMSIZE(inpkt->mbz_itemsize);
	datap = inpkt->u.data;
	if (item_sz > sizeof(cr)) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	bad = FALSE;
	while (items-- > 0 && !bad) {
		memcpy(&cr, datap, item_sz);
		cr.flags = ntohs(cr.flags);
		cr.mflags = ntohs(cr.mflags);
		if (~RESM_NTPONLY & cr.mflags)
			bad |= 1;
		if (~RES_ALLFLAGS & cr.flags)
			bad |= 2;
		if (INADDR_ANY != cr.mask) {
			if (client_v6_capable && cr.v6_flag) {
				if (IN6_IS_ADDR_UNSPECIFIED(&cr.addr6))
					bad |= 4;
			} else {
				if (INADDR_ANY == cr.addr)
					bad |= 8;
			}
		}
		datap += item_sz;
	}

	if (bad) {
		msyslog(LOG_ERR, "do_restrict: bad = %#x", bad);
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	/*
	 * Looks okay, try it out
	 */
	ZERO_SOCK(&matchaddr);
	ZERO_SOCK(&matchmask);
	datap = inpkt->u.data;

	while (items-- > 0) {
		memcpy(&cr, datap, item_sz);
		cr.flags = ntohs(cr.flags);
		cr.mflags = ntohs(cr.mflags);
		if (client_v6_capable && cr.v6_flag) {
			AF(&matchaddr) = AF_INET6;
			AF(&matchmask) = AF_INET6;
			SOCK_ADDR6(&matchaddr) = cr.addr6;
			SOCK_ADDR6(&matchmask) = cr.mask6;
		} else {
			AF(&matchaddr) = AF_INET;
			AF(&matchmask) = AF_INET;
			NSRCADR(&matchaddr) = cr.addr;
			NSRCADR(&matchmask) = cr.mask;
		}
		hack_restrict(op, &matchaddr, &matchmask, cr.mflags,
			      cr.flags, 0);
		datap += item_sz;
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}