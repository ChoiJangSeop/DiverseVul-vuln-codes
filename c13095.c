static void veth_setup(struct net_device *dev)
{
	ether_setup(dev);

	dev->netdev_ops = &veth_netdev_ops;
	dev->ethtool_ops = &veth_ethtool_ops;
	dev->features |= NETIF_F_LLTX;
	dev->destructor = veth_dev_free;

	dev->hw_features = NETIF_F_NO_CSUM | NETIF_F_SG | NETIF_F_RXCSUM;
}