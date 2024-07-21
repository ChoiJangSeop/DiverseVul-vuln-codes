static int ltalk_rcv(struct sk_buff *skb, struct net_device *dev,
			struct packet_type *pt)
{
	/* Expand any short form frames */
	if (skb->mac.raw[2] == 1) {
		struct ddpehdr *ddp;
		/* Find our address */
		struct atalk_addr *ap = atalk_find_dev_addr(dev);

		if (!ap || skb->len < sizeof(struct ddpshdr))
			goto freeit;
		/*
		 * The push leaves us with a ddephdr not an shdr, and
		 * handily the port bytes in the right place preset.
		 */

		skb_push(skb, sizeof(*ddp) - 4);
		/* FIXME: use skb->cb to be able to use shared skbs */
		ddp = (struct ddpehdr *)skb->data;

		/* Now fill in the long header */

	 	/*
	 	 * These two first. The mac overlays the new source/dest
	 	 * network information so we MUST copy these before
	 	 * we write the network numbers !
	 	 */

		ddp->deh_dnode = skb->mac.raw[0];     /* From physical header */
		ddp->deh_snode = skb->mac.raw[1];     /* From physical header */

		ddp->deh_dnet  = ap->s_net;	/* Network number */
		ddp->deh_snet  = ap->s_net;
		ddp->deh_sum   = 0;		/* No checksum */
		/*
		 * Not sure about this bit...
		 */
		ddp->deh_len   = skb->len;
		ddp->deh_hops  = DDP_MAXHOPS;	/* Non routable, so force a drop
						   if we slip up later */
		/* Mend the byte order */
		*((__u16 *)ddp) = htons(*((__u16 *)ddp));
	}
	skb->h.raw = skb->data;

	return atalk_rcv(skb, dev, pt);
freeit:
	kfree_skb(skb);
	return 0;
}