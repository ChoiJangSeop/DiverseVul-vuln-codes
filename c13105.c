void macvlan_common_setup(struct net_device *dev)
{
	ether_setup(dev);

	dev->priv_flags	       &= ~IFF_XMIT_DST_RELEASE;
	dev->netdev_ops		= &macvlan_netdev_ops;
	dev->destructor		= free_netdev;
	dev->header_ops		= &macvlan_hard_header_ops,
	dev->ethtool_ops	= &macvlan_ethtool_ops;
}