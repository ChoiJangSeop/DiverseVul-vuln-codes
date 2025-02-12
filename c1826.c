static int ip6_frag_queue(struct frag_queue *fq, struct sk_buff *skb,
			   struct frag_hdr *fhdr, int nhoff)
{
	struct sk_buff *prev, *next;
	struct net_device *dev;
	int offset, end;
	struct net *net = dev_net(skb_dst(skb)->dev);

	if (fq->q.last_in & INET_FRAG_COMPLETE)
		goto err;

	offset = ntohs(fhdr->frag_off) & ~0x7;
	end = offset + (ntohs(ipv6_hdr(skb)->payload_len) -
			((u8 *)(fhdr + 1) - (u8 *)(ipv6_hdr(skb) + 1)));

	if ((unsigned int)end > IPV6_MAXPLEN) {
		IP6_INC_STATS_BH(net, ip6_dst_idev(skb_dst(skb)),
				 IPSTATS_MIB_INHDRERRORS);
		icmpv6_param_prob(skb, ICMPV6_HDR_FIELD,
				  ((u8 *)&fhdr->frag_off -
				   skb_network_header(skb)));
		return -1;
	}

	if (skb->ip_summed == CHECKSUM_COMPLETE) {
		const unsigned char *nh = skb_network_header(skb);
		skb->csum = csum_sub(skb->csum,
				     csum_partial(nh, (u8 *)(fhdr + 1) - nh,
						  0));
	}

	/* Is this the final fragment? */
	if (!(fhdr->frag_off & htons(IP6_MF))) {
		/* If we already have some bits beyond end
		 * or have different end, the segment is corrupted.
		 */
		if (end < fq->q.len ||
		    ((fq->q.last_in & INET_FRAG_LAST_IN) && end != fq->q.len))
			goto err;
		fq->q.last_in |= INET_FRAG_LAST_IN;
		fq->q.len = end;
	} else {
		/* Check if the fragment is rounded to 8 bytes.
		 * Required by the RFC.
		 */
		if (end & 0x7) {
			/* RFC2460 says always send parameter problem in
			 * this case. -DaveM
			 */
			IP6_INC_STATS_BH(net, ip6_dst_idev(skb_dst(skb)),
					 IPSTATS_MIB_INHDRERRORS);
			icmpv6_param_prob(skb, ICMPV6_HDR_FIELD,
					  offsetof(struct ipv6hdr, payload_len));
			return -1;
		}
		if (end > fq->q.len) {
			/* Some bits beyond end -> corruption. */
			if (fq->q.last_in & INET_FRAG_LAST_IN)
				goto err;
			fq->q.len = end;
		}
	}

	if (end == offset)
		goto err;

	/* Point into the IP datagram 'data' part. */
	if (!pskb_pull(skb, (u8 *) (fhdr + 1) - skb->data))
		goto err;

	if (pskb_trim_rcsum(skb, end - offset))
		goto err;

	/* Find out which fragments are in front and at the back of us
	 * in the chain of fragments so far.  We must know where to put
	 * this fragment, right?
	 */
	prev = fq->q.fragments_tail;
	if (!prev || FRAG6_CB(prev)->offset < offset) {
		next = NULL;
		goto found;
	}
	prev = NULL;
	for(next = fq->q.fragments; next != NULL; next = next->next) {
		if (FRAG6_CB(next)->offset >= offset)
			break;	/* bingo! */
		prev = next;
	}

found:
	/* RFC5722, Section 4, amended by Errata ID : 3089
	 *                          When reassembling an IPv6 datagram, if
	 *   one or more its constituent fragments is determined to be an
	 *   overlapping fragment, the entire datagram (and any constituent
	 *   fragments) MUST be silently discarded.
	 */

	/* Check for overlap with preceding fragment. */
	if (prev &&
	    (FRAG6_CB(prev)->offset + prev->len) > offset)
		goto discard_fq;

	/* Look for overlap with succeeding segment. */
	if (next && FRAG6_CB(next)->offset < end)
		goto discard_fq;

	FRAG6_CB(skb)->offset = offset;

	/* Insert this fragment in the chain of fragments. */
	skb->next = next;
	if (!next)
		fq->q.fragments_tail = skb;
	if (prev)
		prev->next = skb;
	else
		fq->q.fragments = skb;

	dev = skb->dev;
	if (dev) {
		fq->iif = dev->ifindex;
		skb->dev = NULL;
	}
	fq->q.stamp = skb->tstamp;
	fq->q.meat += skb->len;
	add_frag_mem_limit(&fq->q, skb->truesize);

	/* The first fragment.
	 * nhoffset is obtained from the first fragment, of course.
	 */
	if (offset == 0) {
		fq->nhoffset = nhoff;
		fq->q.last_in |= INET_FRAG_FIRST_IN;
	}

	if (fq->q.last_in == (INET_FRAG_FIRST_IN | INET_FRAG_LAST_IN) &&
	    fq->q.meat == fq->q.len)
		return ip6_frag_reasm(fq, prev, dev);

	write_lock(&ip6_frags.lock);
	list_move_tail(&fq->q.lru_list, &fq->q.net->lru_list);
	write_unlock(&ip6_frags.lock);
	return -1;

discard_fq:
	inet_frag_kill(&fq->q, &ip6_frags);
err:
	IP6_INC_STATS(net, ip6_dst_idev(skb_dst(skb)),
		      IPSTATS_MIB_REASMFAILS);
	kfree_skb(skb);
	return -1;
}