void hostap_setup_dev(struct net_device *dev, local_info_t *local,
		      int type)
{
	struct hostap_interface *iface;

	iface = netdev_priv(dev);
	ether_setup(dev);

	/* kernel callbacks */
	if (iface) {
		/* Currently, we point to the proper spy_data only on
		 * the main_dev. This could be fixed. Jean II */
		iface->wireless_data.spy_data = &iface->spy_data;
		dev->wireless_data = &iface->wireless_data;
	}
	dev->wireless_handlers = &hostap_iw_handler_def;
	dev->watchdog_timeo = TX_TIMEOUT;

	switch(type) {
	case HOSTAP_INTERFACE_AP:
		dev->tx_queue_len = 0;	/* use main radio device queue */
		dev->netdev_ops = &hostap_mgmt_netdev_ops;
		dev->type = ARPHRD_IEEE80211;
		dev->header_ops = &hostap_80211_ops;
		break;
	case HOSTAP_INTERFACE_MASTER:
		dev->netdev_ops = &hostap_master_ops;
		break;
	default:
		dev->tx_queue_len = 0;	/* use main radio device queue */
		dev->netdev_ops = &hostap_netdev_ops;
	}

	dev->mtu = local->mtu;


	SET_ETHTOOL_OPS(dev, &prism2_ethtool_ops);

}