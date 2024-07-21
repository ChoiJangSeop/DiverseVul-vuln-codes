static unsigned xenvif_tx_build_gops(struct xenvif *vif, int budget)
{
	struct gnttab_map_grant_ref *gop = vif->tx_map_ops, *request_gop;
	struct sk_buff *skb;
	int ret;

	while (skb_queue_len(&vif->tx_queue) < budget) {
		struct xen_netif_tx_request txreq;
		struct xen_netif_tx_request txfrags[XEN_NETBK_LEGACY_SLOTS_MAX];
		struct xen_netif_extra_info extras[XEN_NETIF_EXTRA_TYPE_MAX-1];
		u16 pending_idx;
		RING_IDX idx;
		int work_to_do;
		unsigned int data_len;
		pending_ring_idx_t index;

		if (vif->tx.sring->req_prod - vif->tx.req_cons >
		    XEN_NETIF_TX_RING_SIZE) {
			netdev_err(vif->dev,
				   "Impossible number of requests. "
				   "req_prod %d, req_cons %d, size %ld\n",
				   vif->tx.sring->req_prod, vif->tx.req_cons,
				   XEN_NETIF_TX_RING_SIZE);
			xenvif_fatal_tx_err(vif);
			continue;
		}

		work_to_do = RING_HAS_UNCONSUMED_REQUESTS(&vif->tx);
		if (!work_to_do)
			break;

		idx = vif->tx.req_cons;
		rmb(); /* Ensure that we see the request before we copy it. */
		memcpy(&txreq, RING_GET_REQUEST(&vif->tx, idx), sizeof(txreq));

		/* Credit-based scheduling. */
		if (txreq.size > vif->remaining_credit &&
		    tx_credit_exceeded(vif, txreq.size))
			break;

		vif->remaining_credit -= txreq.size;

		work_to_do--;
		vif->tx.req_cons = ++idx;

		memset(extras, 0, sizeof(extras));
		if (txreq.flags & XEN_NETTXF_extra_info) {
			work_to_do = xenvif_get_extras(vif, extras,
						       work_to_do);
			idx = vif->tx.req_cons;
			if (unlikely(work_to_do < 0))
				break;
		}

		ret = xenvif_count_requests(vif, &txreq, txfrags, work_to_do);
		if (unlikely(ret < 0))
			break;

		idx += ret;

		if (unlikely(txreq.size < ETH_HLEN)) {
			netdev_dbg(vif->dev,
				   "Bad packet size: %d\n", txreq.size);
			xenvif_tx_err(vif, &txreq, idx);
			break;
		}

		/* No crossing a page as the payload mustn't fragment. */
		if (unlikely((txreq.offset + txreq.size) > PAGE_SIZE)) {
			netdev_err(vif->dev,
				   "txreq.offset: %x, size: %u, end: %lu\n",
				   txreq.offset, txreq.size,
				   (txreq.offset&~PAGE_MASK) + txreq.size);
			xenvif_fatal_tx_err(vif);
			break;
		}

		index = pending_index(vif->pending_cons);
		pending_idx = vif->pending_ring[index];

		data_len = (txreq.size > PKT_PROT_LEN &&
			    ret < XEN_NETBK_LEGACY_SLOTS_MAX) ?
			PKT_PROT_LEN : txreq.size;

		skb = xenvif_alloc_skb(data_len);
		if (unlikely(skb == NULL)) {
			netdev_dbg(vif->dev,
				   "Can't allocate a skb in start_xmit.\n");
			xenvif_tx_err(vif, &txreq, idx);
			break;
		}

		if (extras[XEN_NETIF_EXTRA_TYPE_GSO - 1].type) {
			struct xen_netif_extra_info *gso;
			gso = &extras[XEN_NETIF_EXTRA_TYPE_GSO - 1];

			if (xenvif_set_skb_gso(vif, skb, gso)) {
				/* Failure in xenvif_set_skb_gso is fatal. */
				kfree_skb(skb);
				break;
			}
		}

		xenvif_tx_create_gop(vif, pending_idx, &txreq, gop);

		gop++;

		XENVIF_TX_CB(skb)->pending_idx = pending_idx;

		__skb_put(skb, data_len);

		skb_shinfo(skb)->nr_frags = ret;
		if (data_len < txreq.size) {
			skb_shinfo(skb)->nr_frags++;
			frag_set_pending_idx(&skb_shinfo(skb)->frags[0],
					     pending_idx);
		} else {
			frag_set_pending_idx(&skb_shinfo(skb)->frags[0],
					     INVALID_PENDING_IDX);
		}

		vif->pending_cons++;

		request_gop = xenvif_get_requests(vif, skb, txfrags, gop);
		if (request_gop == NULL) {
			kfree_skb(skb);
			xenvif_tx_err(vif, &txreq, idx);
			break;
		}
		gop = request_gop;

		__skb_queue_tail(&vif->tx_queue, skb);

		vif->tx.req_cons = idx;

		if ((gop-vif->tx_map_ops) >= ARRAY_SIZE(vif->tx_map_ops))
			break;
	}

	return gop - vif->tx_map_ops;
}