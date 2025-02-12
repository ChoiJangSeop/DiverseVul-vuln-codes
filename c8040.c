static void arc_emac_tx_clean(struct net_device *ndev)
{
	struct arc_emac_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	unsigned int i;

	for (i = 0; i < TX_BD_NUM; i++) {
		unsigned int *txbd_dirty = &priv->txbd_dirty;
		struct arc_emac_bd *txbd = &priv->txbd[*txbd_dirty];
		struct buffer_state *tx_buff = &priv->tx_buff[*txbd_dirty];
		struct sk_buff *skb = tx_buff->skb;
		unsigned int info = le32_to_cpu(txbd->info);

		if ((info & FOR_EMAC) || !txbd->data)
			break;

		if (unlikely(info & (DROP | DEFR | LTCL | UFLO))) {
			stats->tx_errors++;
			stats->tx_dropped++;

			if (info & DEFR)
				stats->tx_carrier_errors++;

			if (info & LTCL)
				stats->collisions++;

			if (info & UFLO)
				stats->tx_fifo_errors++;
		} else if (likely(info & FIRST_OR_LAST_MASK)) {
			stats->tx_packets++;
			stats->tx_bytes += skb->len;
		}

		dma_unmap_single(&ndev->dev, dma_unmap_addr(tx_buff, addr),
				 dma_unmap_len(tx_buff, len), DMA_TO_DEVICE);

		/* return the sk_buff to system */
		dev_kfree_skb_irq(skb);

		txbd->data = 0;
		txbd->info = 0;

		*txbd_dirty = (*txbd_dirty + 1) % TX_BD_NUM;
	}

	/* Ensure that txbd_dirty is visible to tx() before checking
	 * for queue stopped.
	 */
	smp_mb();

	if (netif_queue_stopped(ndev) && arc_emac_tx_avail(priv))
		netif_wake_queue(ndev);
}