ssize_t	ip_append_page(struct sock *sk, struct flowi4 *fl4, struct page *page,
		       int offset, size_t size, int flags)
{
	struct inet_sock *inet = inet_sk(sk);
	struct sk_buff *skb;
	struct rtable *rt;
	struct ip_options *opt = NULL;
	struct inet_cork *cork;
	int hh_len;
	int mtu;
	int len;
	int err;
	unsigned int maxfraglen, fragheaderlen, fraggap, maxnonfragsize;

	if (inet->hdrincl)
		return -EPERM;

	if (flags&MSG_PROBE)
		return 0;

	if (skb_queue_empty(&sk->sk_write_queue))
		return -EINVAL;

	cork = &inet->cork.base;
	rt = (struct rtable *)cork->dst;
	if (cork->flags & IPCORK_OPT)
		opt = cork->opt;

	if (!(rt->dst.dev->features&NETIF_F_SG))
		return -EOPNOTSUPP;

	hh_len = LL_RESERVED_SPACE(rt->dst.dev);
	mtu = cork->fragsize;

	fragheaderlen = sizeof(struct iphdr) + (opt ? opt->optlen : 0);
	maxfraglen = ((mtu - fragheaderlen) & ~7) + fragheaderlen;
	maxnonfragsize = ip_sk_ignore_df(sk) ? 0xFFFF : mtu;

	if (cork->length + size > maxnonfragsize - fragheaderlen) {
		ip_local_error(sk, EMSGSIZE, fl4->daddr, inet->inet_dport,
			       mtu - (opt ? opt->optlen : 0));
		return -EMSGSIZE;
	}

	skb = skb_peek_tail(&sk->sk_write_queue);
	if (!skb)
		return -EINVAL;

	if ((size + skb->len > mtu) &&
	    (sk->sk_protocol == IPPROTO_UDP) &&
	    (rt->dst.dev->features & NETIF_F_UFO)) {
		if (skb->ip_summed != CHECKSUM_PARTIAL)
			return -EOPNOTSUPP;

		skb_shinfo(skb)->gso_size = mtu - fragheaderlen;
		skb_shinfo(skb)->gso_type = SKB_GSO_UDP;
	}
	cork->length += size;

	while (size > 0) {
		if (skb_is_gso(skb)) {
			len = size;
		} else {

			/* Check if the remaining data fits into current packet. */
			len = mtu - skb->len;
			if (len < size)
				len = maxfraglen - skb->len;
		}
		if (len <= 0) {
			struct sk_buff *skb_prev;
			int alloclen;

			skb_prev = skb;
			fraggap = skb_prev->len - maxfraglen;

			alloclen = fragheaderlen + hh_len + fraggap + 15;
			skb = sock_wmalloc(sk, alloclen, 1, sk->sk_allocation);
			if (unlikely(!skb)) {
				err = -ENOBUFS;
				goto error;
			}

			/*
			 *	Fill in the control structures
			 */
			skb->ip_summed = CHECKSUM_NONE;
			skb->csum = 0;
			skb_reserve(skb, hh_len);

			/*
			 *	Find where to start putting bytes.
			 */
			skb_put(skb, fragheaderlen + fraggap);
			skb_reset_network_header(skb);
			skb->transport_header = (skb->network_header +
						 fragheaderlen);
			if (fraggap) {
				skb->csum = skb_copy_and_csum_bits(skb_prev,
								   maxfraglen,
						    skb_transport_header(skb),
								   fraggap, 0);
				skb_prev->csum = csum_sub(skb_prev->csum,
							  skb->csum);
				pskb_trim_unique(skb_prev, maxfraglen);
			}

			/*
			 * Put the packet on the pending queue.
			 */
			__skb_queue_tail(&sk->sk_write_queue, skb);
			continue;
		}

		if (len > size)
			len = size;

		if (skb_append_pagefrags(skb, page, offset, len)) {
			err = -EMSGSIZE;
			goto error;
		}

		if (skb->ip_summed == CHECKSUM_NONE) {
			__wsum csum;
			csum = csum_page(page, offset, len);
			skb->csum = csum_block_add(skb->csum, csum, skb->len);
		}

		skb->len += len;
		skb->data_len += len;
		skb->truesize += len;
		refcount_add(len, &sk->sk_wmem_alloc);
		offset += len;
		size -= len;
	}
	return 0;

error:
	cork->length -= size;
	IP_INC_STATS(sock_net(sk), IPSTATS_MIB_OUTDISCARDS);
	return err;
}