static void l2tp_eth_dev_setup(struct net_device *dev)
{
	ether_setup(dev);

	dev->netdev_ops		= &l2tp_eth_netdev_ops;
	dev->destructor		= free_netdev;
}