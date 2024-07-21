do_trustkey(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt,
	u_long trust
	)
{
	register u_long *kp;
	register int items;

	items = INFO_NITEMS(inpkt->err_nitems);
	kp = (u_long *)&inpkt->u;
	while (items-- > 0) {
		authtrust(*kp, trust);
		kp++;
	}

	req_ack(srcadr, inter, inpkt, INFO_OKAY);
}