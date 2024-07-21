reset_peer(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt
	)
{
	u_short			items;
	size_t			item_sz;
	char *			datap;
	struct conf_unpeer	cp;
	struct peer *		p;
	sockaddr_u		peeraddr;
	int			bad;

	/*
	 * We check first to see that every peer exists.  If not,
	 * we return an error.
	 */

	items = INFO_NITEMS(inpkt->err_nitems);
	item_sz = INFO_ITEMSIZE(inpkt->mbz_itemsize);
	datap = inpkt->u.data;
	if (item_sz > sizeof(cp)) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_FMT);
		return;
	}

	bad = FALSE;
	while (items-- > 0 && !bad) {
		ZERO(cp);
		memcpy(&cp, datap, item_sz);
		ZERO_SOCK(&peeraddr);
		if (client_v6_capable && cp.v6_flag) {
			AF(&peeraddr) = AF_INET6;
			SOCK_ADDR6(&peeraddr) = cp.peeraddr6;
		} else {
			AF(&peeraddr) = AF_INET;
			NSRCADR(&peeraddr) = cp.peeraddr;
		}

#ifdef ISC_PLATFORM_HAVESALEN
		peeraddr.sa.sa_len = SOCKLEN(&peeraddr);
#endif
		p = findexistingpeer(&peeraddr, NULL, NULL, -1, 0);
		if (NULL == p)
			bad++;
		datap += item_sz;
	}

	if (bad) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
		return;
	}

	/*
	 * Now do it in earnest.
	 */

	datap = inpkt->u.data;
	while (items-- > 0) {
		ZERO(cp);
		memcpy(&cp, datap, item_sz);
		ZERO_SOCK(&peeraddr);
		if (client_v6_capable && cp.v6_flag) {
			AF(&peeraddr) = AF_INET6;
			SOCK_ADDR6(&peeraddr) = cp.peeraddr6;
		} else {
			AF(&peeraddr) = AF_INET;
			NSRCADR(&peeraddr) = cp.peeraddr;
		}
		SET_PORT(&peeraddr, 123);
#ifdef ISC_PLATFORM_HAVESALEN
		peeraddr.sa.sa_len = SOCKLEN(&peeraddr);
#endif
		p = findexistingpeer(&peeraddr, NULL, NULL, -1, 0);
		while (p != NULL) {
			peer_reset(p);
			p = findexistingpeer(&peeraddr, NULL, p, -1, 0);
		}
		datap += item_sz;
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}