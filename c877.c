struct sk_buff *nf_ct_frag6_gather(struct sk_buff *skb, u32 user)
{
	struct sk_buff *clone;
	struct net_device *dev = skb->dev;
	struct frag_hdr *fhdr;
	struct nf_ct_frag6_queue *fq;
	struct ipv6hdr *hdr;
	int fhoff, nhoff;
	u8 prevhdr;
	struct sk_buff *ret_skb = NULL;

	/* Jumbo payload inhibits frag. header */
	if (ipv6_hdr(skb)->payload_len == 0) {
		pr_debug("payload len = 0\n");
		return skb;
	}

	if (find_prev_fhdr(skb, &prevhdr, &nhoff, &fhoff) < 0)
		return skb;

	clone = skb_clone(skb, GFP_ATOMIC);
	if (clone == NULL) {
		pr_debug("Can't clone skb\n");
		return skb;
	}

	NFCT_FRAG6_CB(clone)->orig = skb;

	if (!pskb_may_pull(clone, fhoff + sizeof(*fhdr))) {
		pr_debug("message is too short.\n");
		goto ret_orig;
	}

	skb_set_transport_header(clone, fhoff);
	hdr = ipv6_hdr(clone);
	fhdr = (struct frag_hdr *)skb_transport_header(clone);

	if (!(fhdr->frag_off & htons(0xFFF9))) {
		pr_debug("Invalid fragment offset\n");
		/* It is not a fragmented frame */
		goto ret_orig;
	}

	if (atomic_read(&nf_init_frags.mem) > nf_init_frags.high_thresh)
		nf_ct_frag6_evictor();

	fq = fq_find(fhdr->identification, user, &hdr->saddr, &hdr->daddr);
	if (fq == NULL) {
		pr_debug("Can't find and can't create new queue\n");
		goto ret_orig;
	}

	spin_lock_bh(&fq->q.lock);

	if (nf_ct_frag6_queue(fq, clone, fhdr, nhoff) < 0) {
		spin_unlock_bh(&fq->q.lock);
		pr_debug("Can't insert skb to queue\n");
		fq_put(fq);
		goto ret_orig;
	}

	if (fq->q.last_in == (INET_FRAG_FIRST_IN | INET_FRAG_LAST_IN) &&
	    fq->q.meat == fq->q.len) {
		ret_skb = nf_ct_frag6_reasm(fq, dev);
		if (ret_skb == NULL)
			pr_debug("Can't reassemble fragmented packets\n");
	}
	spin_unlock_bh(&fq->q.lock);

	fq_put(fq);
	return ret_skb;

ret_orig:
	kfree_skb(clone);
	return skb;
}