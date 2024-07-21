int ip6_append_data(struct sock *sk, int getfrag(void *from, char *to,
	int offset, int len, int odd, struct sk_buff *skb),
	void *from, int length, int transhdrlen,
	int hlimit, int tclass, struct ipv6_txoptions *opt, struct flowi *fl,
	struct rt6_info *rt, unsigned int flags)
{
	struct inet_sock *inet = inet_sk(sk);
	struct ipv6_pinfo *np = inet6_sk(sk);
	struct sk_buff *skb;
	unsigned int maxfraglen, fragheaderlen;
	int exthdrlen;
	int hh_len;
	int mtu;
	int copy;
	int err;
	int offset = 0;
	int csummode = CHECKSUM_NONE;

	if (flags&MSG_PROBE)
		return 0;
	if (skb_queue_empty(&sk->sk_write_queue)) {
		/*
		 * setup for corking
		 */
		if (opt) {
			if (np->cork.opt == NULL) {
				np->cork.opt = kmalloc(opt->tot_len,
						       sk->sk_allocation);
				if (unlikely(np->cork.opt == NULL))
					return -ENOBUFS;
			} else if (np->cork.opt->tot_len < opt->tot_len) {
				printk(KERN_DEBUG "ip6_append_data: invalid option length\n");
				return -EINVAL;
			}
			memcpy(np->cork.opt, opt, opt->tot_len);
			inet->cork.flags |= IPCORK_OPT;
			/* need source address above miyazawa*/
		}
		dst_hold(&rt->u.dst);
		np->cork.rt = rt;
		inet->cork.fl = *fl;
		np->cork.hop_limit = hlimit;
		np->cork.tclass = tclass;
		inet->cork.fragsize = mtu = dst_mtu(rt->u.dst.path);
		if (dst_allfrag(rt->u.dst.path))
			inet->cork.flags |= IPCORK_ALLFRAG;
		inet->cork.length = 0;
		sk->sk_sndmsg_page = NULL;
		sk->sk_sndmsg_off = 0;
		exthdrlen = rt->u.dst.header_len + (opt ? opt->opt_flen : 0);
		length += exthdrlen;
		transhdrlen += exthdrlen;
	} else {
		rt = np->cork.rt;
		fl = &inet->cork.fl;
		if (inet->cork.flags & IPCORK_OPT)
			opt = np->cork.opt;
		transhdrlen = 0;
		exthdrlen = 0;
		mtu = inet->cork.fragsize;
	}

	hh_len = LL_RESERVED_SPACE(rt->u.dst.dev);

	fragheaderlen = sizeof(struct ipv6hdr) + (opt ? opt->opt_nflen : 0);
	maxfraglen = ((mtu - fragheaderlen) & ~7) + fragheaderlen - sizeof(struct frag_hdr);

	if (mtu <= sizeof(struct ipv6hdr) + IPV6_MAXPLEN) {
		if (inet->cork.length + length > sizeof(struct ipv6hdr) + IPV6_MAXPLEN - fragheaderlen) {
			ipv6_local_error(sk, EMSGSIZE, fl, mtu-exthdrlen);
			return -EMSGSIZE;
		}
	}

	/*
	 * Let's try using as much space as possible.
	 * Use MTU if total length of the message fits into the MTU.
	 * Otherwise, we need to reserve fragment header and
	 * fragment alignment (= 8-15 octects, in total).
	 *
	 * Note that we may need to "move" the data from the tail of
	 * of the buffer to the new fragment when we split 
	 * the message.
	 *
	 * FIXME: It may be fragmented into multiple chunks 
	 *        at once if non-fragmentable extension headers
	 *        are too large.
	 * --yoshfuji 
	 */

	inet->cork.length += length;

	if ((skb = skb_peek_tail(&sk->sk_write_queue)) == NULL)
		goto alloc_new_skb;

	while (length > 0) {
		/* Check if the remaining data fits into current packet. */
		copy = (inet->cork.length <= mtu && !(inet->cork.flags & IPCORK_ALLFRAG) ? mtu : maxfraglen) - skb->len;
		if (copy < length)
			copy = maxfraglen - skb->len;

		if (copy <= 0) {
			char *data;
			unsigned int datalen;
			unsigned int fraglen;
			unsigned int fraggap;
			unsigned int alloclen;
			struct sk_buff *skb_prev;
alloc_new_skb:
			skb_prev = skb;

			/* There's no room in the current skb */
			if (skb_prev)
				fraggap = skb_prev->len - maxfraglen;
			else
				fraggap = 0;

			/*
			 * If remaining data exceeds the mtu,
			 * we know we need more fragment(s).
			 */
			datalen = length + fraggap;
			if (datalen > (inet->cork.length <= mtu && !(inet->cork.flags & IPCORK_ALLFRAG) ? mtu : maxfraglen) - fragheaderlen)
				datalen = maxfraglen - fragheaderlen;

			fraglen = datalen + fragheaderlen;
			if ((flags & MSG_MORE) &&
			    !(rt->u.dst.dev->features&NETIF_F_SG))
				alloclen = mtu;
			else
				alloclen = datalen + fragheaderlen;

			/*
			 * The last fragment gets additional space at tail.
			 * Note: we overallocate on fragments with MSG_MODE
			 * because we have no idea if we're the last one.
			 */
			if (datalen == length + fraggap)
				alloclen += rt->u.dst.trailer_len;

			/*
			 * We just reserve space for fragment header.
			 * Note: this may be overallocation if the message 
			 * (without MSG_MORE) fits into the MTU.
			 */
			alloclen += sizeof(struct frag_hdr);

			if (transhdrlen) {
				skb = sock_alloc_send_skb(sk,
						alloclen + hh_len,
						(flags & MSG_DONTWAIT), &err);
			} else {
				skb = NULL;
				if (atomic_read(&sk->sk_wmem_alloc) <=
				    2 * sk->sk_sndbuf)
					skb = sock_wmalloc(sk,
							   alloclen + hh_len, 1,
							   sk->sk_allocation);
				if (unlikely(skb == NULL))
					err = -ENOBUFS;
			}
			if (skb == NULL)
				goto error;
			/*
			 *	Fill in the control structures
			 */
			skb->ip_summed = csummode;
			skb->csum = 0;
			/* reserve for fragmentation */
			skb_reserve(skb, hh_len+sizeof(struct frag_hdr));

			/*
			 *	Find where to start putting bytes
			 */
			data = skb_put(skb, fraglen);
			skb->nh.raw = data + exthdrlen;
			data += fragheaderlen;
			skb->h.raw = data + exthdrlen;

			if (fraggap) {
				skb->csum = skb_copy_and_csum_bits(
					skb_prev, maxfraglen,
					data + transhdrlen, fraggap, 0);
				skb_prev->csum = csum_sub(skb_prev->csum,
							  skb->csum);
				data += fraggap;
				skb_trim(skb_prev, maxfraglen);
			}
			copy = datalen - transhdrlen - fraggap;
			if (copy < 0) {
				err = -EINVAL;
				kfree_skb(skb);
				goto error;
			} else if (copy > 0 && getfrag(from, data + transhdrlen, offset, copy, fraggap, skb) < 0) {
				err = -EFAULT;
				kfree_skb(skb);
				goto error;
			}

			offset += copy;
			length -= datalen - fraggap;
			transhdrlen = 0;
			exthdrlen = 0;
			csummode = CHECKSUM_NONE;

			/*
			 * Put the packet on the pending queue
			 */
			__skb_queue_tail(&sk->sk_write_queue, skb);
			continue;
		}

		if (copy > length)
			copy = length;

		if (!(rt->u.dst.dev->features&NETIF_F_SG)) {
			unsigned int off;

			off = skb->len;
			if (getfrag(from, skb_put(skb, copy),
						offset, copy, off, skb) < 0) {
				__skb_trim(skb, off);
				err = -EFAULT;
				goto error;
			}
		} else {
			int i = skb_shinfo(skb)->nr_frags;
			skb_frag_t *frag = &skb_shinfo(skb)->frags[i-1];
			struct page *page = sk->sk_sndmsg_page;
			int off = sk->sk_sndmsg_off;
			unsigned int left;

			if (page && (left = PAGE_SIZE - off) > 0) {
				if (copy >= left)
					copy = left;
				if (page != frag->page) {
					if (i == MAX_SKB_FRAGS) {
						err = -EMSGSIZE;
						goto error;
					}
					get_page(page);
					skb_fill_page_desc(skb, i, page, sk->sk_sndmsg_off, 0);
					frag = &skb_shinfo(skb)->frags[i];
				}
			} else if(i < MAX_SKB_FRAGS) {
				if (copy > PAGE_SIZE)
					copy = PAGE_SIZE;
				page = alloc_pages(sk->sk_allocation, 0);
				if (page == NULL) {
					err = -ENOMEM;
					goto error;
				}
				sk->sk_sndmsg_page = page;
				sk->sk_sndmsg_off = 0;

				skb_fill_page_desc(skb, i, page, 0, 0);
				frag = &skb_shinfo(skb)->frags[i];
				skb->truesize += PAGE_SIZE;
				atomic_add(PAGE_SIZE, &sk->sk_wmem_alloc);
			} else {
				err = -EMSGSIZE;
				goto error;
			}
			if (getfrag(from, page_address(frag->page)+frag->page_offset+frag->size, offset, copy, skb->len, skb) < 0) {
				err = -EFAULT;
				goto error;
			}
			sk->sk_sndmsg_off += copy;
			frag->size += copy;
			skb->len += copy;
			skb->data_len += copy;
		}
		offset += copy;
		length -= copy;
	}
	return 0;
error:
	inet->cork.length -= length;
	IP6_INC_STATS(IPSTATS_MIB_OUTDISCARDS);
	return err;
}