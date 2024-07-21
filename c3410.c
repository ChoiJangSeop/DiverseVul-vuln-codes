list_restrict(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt
	)
{
	struct info_restrict *ir;

	DPRINTF(3, ("wants restrict list summary\n"));

	ir = (struct info_restrict *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_restrict));
	
	/*
	 * The restriction lists are kept sorted in the reverse order
	 * than they were originally.  To preserve the output semantics,
	 * dump each list in reverse order.  A recursive helper function
	 * achieves that.
	 */
	list_restrict4(restrictlist4, &ir);
	if (client_v6_capable)
		list_restrict6(restrictlist6, &ir);
	flush_pkt();
}