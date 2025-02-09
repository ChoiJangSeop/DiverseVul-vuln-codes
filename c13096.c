static void ieee80211_if_setup(struct net_device *dev)
{
	ether_setup(dev);
	dev->netdev_ops = &ieee80211_dataif_ops;
	dev->destructor = free_netdev;
}