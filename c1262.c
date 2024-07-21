static netdev_tx_t veth_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct net_device *rcv = NULL;
	struct veth_priv *priv, *rcv_priv;
	struct veth_net_stats *stats, *rcv_stats;
	int length;

	priv = netdev_priv(dev);
	rcv = priv->peer;
	rcv_priv = netdev_priv(rcv);

	stats = this_cpu_ptr(priv->stats);
	rcv_stats = this_cpu_ptr(rcv_priv->stats);

	if (!(rcv->flags & IFF_UP))
		goto tx_drop;

	if (dev->features & NETIF_F_NO_CSUM)
		skb->ip_summed = rcv_priv->ip_summed;

	length = skb->len + ETH_HLEN;
	if (dev_forward_skb(rcv, skb) != NET_RX_SUCCESS)
		goto rx_drop;

	stats->tx_bytes += length;
	stats->tx_packets++;

	rcv_stats->rx_bytes += length;
	rcv_stats->rx_packets++;

	return NETDEV_TX_OK;

tx_drop:
	kfree_skb(skb);
	stats->tx_dropped++;
	return NETDEV_TX_OK;

rx_drop:
	kfree_skb(skb);
	rcv_stats->rx_dropped++;
	return NETDEV_TX_OK;
}