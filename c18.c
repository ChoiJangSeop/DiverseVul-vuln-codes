static int atalk_rcv(struct sk_buff *skb, struct net_device *dev,
		     struct packet_type *pt)
{
	struct ddpehdr *ddp = ddp_hdr(skb);
	struct sock *sock;
	struct atalk_iface *atif;
	struct sockaddr_at tosat;
        int origlen;
        struct ddpebits ddphv;

	/* Size check */
	if (skb->len < sizeof(*ddp))
		goto freeit;

	/*
	 *	Fix up the length field	[Ok this is horrible but otherwise
	 *	I end up with unions of bit fields and messy bit field order
	 *	compiler/endian dependencies..]
	 *
	 *	FIXME: This is a write to a shared object. Granted it
	 *	happens to be safe BUT.. (Its safe as user space will not
	 *	run until we put it back)
	 */
	*((__u16 *)&ddphv) = ntohs(*((__u16 *)ddp));

	/* Trim buffer in case of stray trailing data */
	origlen = skb->len;
	skb_trim(skb, min_t(unsigned int, skb->len, ddphv.deh_len));

	/*
	 * Size check to see if ddp->deh_len was crap
	 * (Otherwise we'll detonate most spectacularly
	 * in the middle of recvmsg()).
	 */
	if (skb->len < sizeof(*ddp))
		goto freeit;

	/*
	 * Any checksums. Note we don't do htons() on this == is assumed to be
	 * valid for net byte orders all over the networking code...
	 */
	if (ddp->deh_sum &&
	    atalk_checksum(ddp, ddphv.deh_len) != ddp->deh_sum)
		/* Not a valid AppleTalk frame - dustbin time */
		goto freeit;

	/* Check the packet is aimed at us */
	if (!ddp->deh_dnet)	/* Net 0 is 'this network' */
		atif = atalk_find_anynet(ddp->deh_dnode, dev);
	else
		atif = atalk_find_interface(ddp->deh_dnet, ddp->deh_dnode);

	/* Not ours, so we route the packet via the correct AppleTalk iface */
	if (!atif) {
		atalk_route_packet(skb, dev, ddp, &ddphv, origlen);
		goto out;
	}

	/* if IP over DDP is not selected this code will be optimized out */
	if (is_ip_over_ddp(skb))
		return handle_ip_over_ddp(skb);
	/*
	 * Which socket - atalk_search_socket() looks for a *full match*
	 * of the <net, node, port> tuple.
	 */
	tosat.sat_addr.s_net  = ddp->deh_dnet;
	tosat.sat_addr.s_node = ddp->deh_dnode;
	tosat.sat_port	      = ddp->deh_dport;

	sock = atalk_search_socket(&tosat, atif);
	if (!sock) /* But not one of our sockets */
		goto freeit;

	/* Queue packet (standard) */
	skb->sk = sock;

	if (sock_queue_rcv_skb(sock, skb) < 0)
		goto freeit;
out:
	return 0;
freeit:
	kfree_skb(skb);
	goto out;
}