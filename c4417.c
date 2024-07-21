static int fwnet_incoming_packet(struct fwnet_device *dev, __be32 *buf, int len,
				 int source_node_id, int generation,
				 bool is_broadcast)
{
	struct sk_buff *skb;
	struct net_device *net = dev->netdev;
	struct rfc2734_header hdr;
	unsigned lf;
	unsigned long flags;
	struct fwnet_peer *peer;
	struct fwnet_partial_datagram *pd;
	int fg_off;
	int dg_size;
	u16 datagram_label;
	int retval;
	u16 ether_type;

	hdr.w0 = be32_to_cpu(buf[0]);
	lf = fwnet_get_hdr_lf(&hdr);
	if (lf == RFC2374_HDR_UNFRAG) {
		/*
		 * An unfragmented datagram has been received by the ieee1394
		 * bus. Build an skbuff around it so we can pass it to the
		 * high level network layer.
		 */
		ether_type = fwnet_get_hdr_ether_type(&hdr);
		buf++;
		len -= RFC2374_UNFRAG_HDR_SIZE;

		skb = dev_alloc_skb(len + LL_RESERVED_SPACE(net));
		if (unlikely(!skb)) {
			net->stats.rx_dropped++;

			return -ENOMEM;
		}
		skb_reserve(skb, LL_RESERVED_SPACE(net));
		memcpy(skb_put(skb, len), buf, len);

		return fwnet_finish_incoming_packet(net, skb, source_node_id,
						    is_broadcast, ether_type);
	}
	/* A datagram fragment has been received, now the fun begins. */
	hdr.w1 = ntohl(buf[1]);
	buf += 2;
	len -= RFC2374_FRAG_HDR_SIZE;
	if (lf == RFC2374_HDR_FIRSTFRAG) {
		ether_type = fwnet_get_hdr_ether_type(&hdr);
		fg_off = 0;
	} else {
		ether_type = 0;
		fg_off = fwnet_get_hdr_fg_off(&hdr);
	}
	datagram_label = fwnet_get_hdr_dgl(&hdr);
	dg_size = fwnet_get_hdr_dg_size(&hdr); /* ??? + 1 */

	spin_lock_irqsave(&dev->lock, flags);

	peer = fwnet_peer_find_by_node_id(dev, source_node_id, generation);
	if (!peer) {
		retval = -ENOENT;
		goto fail;
	}

	pd = fwnet_pd_find(peer, datagram_label);
	if (pd == NULL) {
		while (peer->pdg_size >= FWNET_MAX_FRAGMENTS) {
			/* remove the oldest */
			fwnet_pd_delete(list_first_entry(&peer->pd_list,
				struct fwnet_partial_datagram, pd_link));
			peer->pdg_size--;
		}
		pd = fwnet_pd_new(net, peer, datagram_label,
				  dg_size, buf, fg_off, len);
		if (pd == NULL) {
			retval = -ENOMEM;
			goto fail;
		}
		peer->pdg_size++;
	} else {
		if (fwnet_frag_overlap(pd, fg_off, len) ||
		    pd->datagram_size != dg_size) {
			/*
			 * Differing datagram sizes or overlapping fragments,
			 * discard old datagram and start a new one.
			 */
			fwnet_pd_delete(pd);
			pd = fwnet_pd_new(net, peer, datagram_label,
					  dg_size, buf, fg_off, len);
			if (pd == NULL) {
				peer->pdg_size--;
				retval = -ENOMEM;
				goto fail;
			}
		} else {
			if (!fwnet_pd_update(peer, pd, buf, fg_off, len)) {
				/*
				 * Couldn't save off fragment anyway
				 * so might as well obliterate the
				 * datagram now.
				 */
				fwnet_pd_delete(pd);
				peer->pdg_size--;
				retval = -ENOMEM;
				goto fail;
			}
		}
	} /* new datagram or add to existing one */

	if (lf == RFC2374_HDR_FIRSTFRAG)
		pd->ether_type = ether_type;

	if (fwnet_pd_is_complete(pd)) {
		ether_type = pd->ether_type;
		peer->pdg_size--;
		skb = skb_get(pd->skb);
		fwnet_pd_delete(pd);

		spin_unlock_irqrestore(&dev->lock, flags);

		return fwnet_finish_incoming_packet(net, skb, source_node_id,
						    false, ether_type);
	}
	/*
	 * Datagram is not complete, we're done for the
	 * moment.
	 */
	retval = 0;
 fail:
	spin_unlock_irqrestore(&dev->lock, flags);

	return retval;
}