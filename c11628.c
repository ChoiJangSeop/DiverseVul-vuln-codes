static int xennet_get_responses(struct netfront_queue *queue,
				struct netfront_rx_info *rinfo, RING_IDX rp,
				struct sk_buff_head *list,
				bool *need_xdp_flush)
{
	struct xen_netif_rx_response *rx = &rinfo->rx, rx_local;
	int max = XEN_NETIF_NR_SLOTS_MIN + (rx->status <= RX_COPY_THRESHOLD);
	RING_IDX cons = queue->rx.rsp_cons;
	struct sk_buff *skb = xennet_get_rx_skb(queue, cons);
	struct xen_netif_extra_info *extras = rinfo->extras;
	grant_ref_t ref = xennet_get_rx_ref(queue, cons);
	struct device *dev = &queue->info->netdev->dev;
	struct bpf_prog *xdp_prog;
	struct xdp_buff xdp;
	int slots = 1;
	int err = 0;
	u32 verdict;

	if (rx->flags & XEN_NETRXF_extra_info) {
		err = xennet_get_extras(queue, extras, rp);
		if (!err) {
			if (extras[XEN_NETIF_EXTRA_TYPE_XDP - 1].type) {
				struct xen_netif_extra_info *xdp;

				xdp = &extras[XEN_NETIF_EXTRA_TYPE_XDP - 1];
				rx->offset = xdp->u.xdp.headroom;
			}
		}
		cons = queue->rx.rsp_cons;
	}

	for (;;) {
		if (unlikely(rx->status < 0 ||
			     rx->offset + rx->status > XEN_PAGE_SIZE)) {
			if (net_ratelimit())
				dev_warn(dev, "rx->offset: %u, size: %d\n",
					 rx->offset, rx->status);
			xennet_move_rx_slot(queue, skb, ref);
			err = -EINVAL;
			goto next;
		}

		/*
		 * This definitely indicates a bug, either in this driver or in
		 * the backend driver. In future this should flag the bad
		 * situation to the system controller to reboot the backend.
		 */
		if (ref == INVALID_GRANT_REF) {
			if (net_ratelimit())
				dev_warn(dev, "Bad rx response id %d.\n",
					 rx->id);
			err = -EINVAL;
			goto next;
		}

		if (!gnttab_end_foreign_access_ref(ref)) {
			dev_alert(dev,
				  "Grant still in use by backend domain\n");
			queue->info->broken = true;
			dev_alert(dev, "Disabled for further use\n");
			return -EINVAL;
		}

		gnttab_release_grant_reference(&queue->gref_rx_head, ref);

		rcu_read_lock();
		xdp_prog = rcu_dereference(queue->xdp_prog);
		if (xdp_prog) {
			if (!(rx->flags & XEN_NETRXF_more_data)) {
				/* currently only a single page contains data */
				verdict = xennet_run_xdp(queue,
							 skb_frag_page(&skb_shinfo(skb)->frags[0]),
							 rx, xdp_prog, &xdp, need_xdp_flush);
				if (verdict != XDP_PASS)
					err = -EINVAL;
			} else {
				/* drop the frame */
				err = -EINVAL;
			}
		}
		rcu_read_unlock();
next:
		__skb_queue_tail(list, skb);
		if (!(rx->flags & XEN_NETRXF_more_data))
			break;

		if (cons + slots == rp) {
			if (net_ratelimit())
				dev_warn(dev, "Need more slots\n");
			err = -ENOENT;
			break;
		}

		RING_COPY_RESPONSE(&queue->rx, cons + slots, &rx_local);
		rx = &rx_local;
		skb = xennet_get_rx_skb(queue, cons + slots);
		ref = xennet_get_rx_ref(queue, cons + slots);
		slots++;
	}

	if (unlikely(slots > max)) {
		if (net_ratelimit())
			dev_warn(dev, "Too many slots\n");
		err = -E2BIG;
	}

	if (unlikely(err))
		xennet_set_rx_rsp_cons(queue, cons + slots);

	return err;
}