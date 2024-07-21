static int gfar_clean_rx_ring(struct gfar_priv_rx_q *rx_queue,
			      int rx_work_limit)
{
	struct net_device *ndev = rx_queue->ndev;
	struct gfar_private *priv = netdev_priv(ndev);
	struct rxbd8 *bdp;
	int i, howmany = 0;
	struct sk_buff *skb = rx_queue->skb;
	int cleaned_cnt = gfar_rxbd_unused(rx_queue);
	unsigned int total_bytes = 0, total_pkts = 0;

	/* Get the first full descriptor */
	i = rx_queue->next_to_clean;

	while (rx_work_limit--) {
		u32 lstatus;

		if (cleaned_cnt >= GFAR_RX_BUFF_ALLOC) {
			gfar_alloc_rx_buffs(rx_queue, cleaned_cnt);
			cleaned_cnt = 0;
		}

		bdp = &rx_queue->rx_bd_base[i];
		lstatus = be32_to_cpu(bdp->lstatus);
		if (lstatus & BD_LFLAG(RXBD_EMPTY))
			break;

		/* order rx buffer descriptor reads */
		rmb();

		/* fetch next to clean buffer from the ring */
		skb = gfar_get_next_rxbuff(rx_queue, lstatus, skb);
		if (unlikely(!skb))
			break;

		cleaned_cnt++;
		howmany++;

		if (unlikely(++i == rx_queue->rx_ring_size))
			i = 0;

		rx_queue->next_to_clean = i;

		/* fetch next buffer if not the last in frame */
		if (!(lstatus & BD_LFLAG(RXBD_LAST)))
			continue;

		if (unlikely(lstatus & BD_LFLAG(RXBD_ERR))) {
			count_errors(lstatus, ndev);

			/* discard faulty buffer */
			dev_kfree_skb(skb);
			skb = NULL;
			rx_queue->stats.rx_dropped++;
			continue;
		}

		gfar_process_frame(ndev, skb);

		/* Increment the number of packets */
		total_pkts++;
		total_bytes += skb->len;

		skb_record_rx_queue(skb, rx_queue->qindex);

		skb->protocol = eth_type_trans(skb, ndev);

		/* Send the packet up the stack */
		napi_gro_receive(&rx_queue->grp->napi_rx, skb);

		skb = NULL;
	}

	/* Store incomplete frames for completion */
	rx_queue->skb = skb;

	rx_queue->stats.rx_packets += total_pkts;
	rx_queue->stats.rx_bytes += total_bytes;

	if (cleaned_cnt)
		gfar_alloc_rx_buffs(rx_queue, cleaned_cnt);

	/* Update Last Free RxBD pointer for LFC */
	if (unlikely(priv->tx_actual_en)) {
		u32 bdp_dma = gfar_rxbd_dma_lastfree(rx_queue);

		gfar_write(rx_queue->rfbptr, bdp_dma);
	}

	return howmany;
}