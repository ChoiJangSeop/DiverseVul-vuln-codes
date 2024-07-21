int dev_ethtool(struct ifreq *ifr)
{
	struct net_device *dev = __dev_get_by_name(ifr->ifr_name);
	void __user *useraddr = ifr->ifr_data;
	u32 ethcmd;
	int rc;
	unsigned long old_features;

	/*
	 * XXX: This can be pushed down into the ethtool_* handlers that
	 * need it.  Keep existing behaviour for the moment.
	 */
	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!dev || !netif_device_present(dev))
		return -ENODEV;

	if (!dev->ethtool_ops)
		goto ioctl;

	if (copy_from_user(&ethcmd, useraddr, sizeof (ethcmd)))
		return -EFAULT;

	if(dev->ethtool_ops->begin)
		if ((rc = dev->ethtool_ops->begin(dev)) < 0)
			return rc;

	old_features = dev->features;

	switch (ethcmd) {
	case ETHTOOL_GSET:
		rc = ethtool_get_settings(dev, useraddr);
		break;
	case ETHTOOL_SSET:
		rc = ethtool_set_settings(dev, useraddr);
		break;
	case ETHTOOL_GDRVINFO:
		rc = ethtool_get_drvinfo(dev, useraddr);
		break;
	case ETHTOOL_GREGS:
		rc = ethtool_get_regs(dev, useraddr);
		break;
	case ETHTOOL_GWOL:
		rc = ethtool_get_wol(dev, useraddr);
		break;
	case ETHTOOL_SWOL:
		rc = ethtool_set_wol(dev, useraddr);
		break;
	case ETHTOOL_GMSGLVL:
		rc = ethtool_get_msglevel(dev, useraddr);
		break;
	case ETHTOOL_SMSGLVL:
		rc = ethtool_set_msglevel(dev, useraddr);
		break;
	case ETHTOOL_NWAY_RST:
		rc = ethtool_nway_reset(dev);
		break;
	case ETHTOOL_GLINK:
		rc = ethtool_get_link(dev, useraddr);
		break;
	case ETHTOOL_GEEPROM:
		rc = ethtool_get_eeprom(dev, useraddr);
		break;
	case ETHTOOL_SEEPROM:
		rc = ethtool_set_eeprom(dev, useraddr);
		break;
	case ETHTOOL_GCOALESCE:
		rc = ethtool_get_coalesce(dev, useraddr);
		break;
	case ETHTOOL_SCOALESCE:
		rc = ethtool_set_coalesce(dev, useraddr);
		break;
	case ETHTOOL_GRINGPARAM:
		rc = ethtool_get_ringparam(dev, useraddr);
		break;
	case ETHTOOL_SRINGPARAM:
		rc = ethtool_set_ringparam(dev, useraddr);
		break;
	case ETHTOOL_GPAUSEPARAM:
		rc = ethtool_get_pauseparam(dev, useraddr);
		break;
	case ETHTOOL_SPAUSEPARAM:
		rc = ethtool_set_pauseparam(dev, useraddr);
		break;
	case ETHTOOL_GRXCSUM:
		rc = ethtool_get_rx_csum(dev, useraddr);
		break;
	case ETHTOOL_SRXCSUM:
		rc = ethtool_set_rx_csum(dev, useraddr);
		break;
	case ETHTOOL_GTXCSUM:
		rc = ethtool_get_tx_csum(dev, useraddr);
		break;
	case ETHTOOL_STXCSUM:
		rc = ethtool_set_tx_csum(dev, useraddr);
		break;
	case ETHTOOL_GSG:
		rc = ethtool_get_sg(dev, useraddr);
		break;
	case ETHTOOL_SSG:
		rc = ethtool_set_sg(dev, useraddr);
		break;
	case ETHTOOL_GTSO:
		rc = ethtool_get_tso(dev, useraddr);
		break;
	case ETHTOOL_STSO:
		rc = ethtool_set_tso(dev, useraddr);
		break;
	case ETHTOOL_TEST:
		rc = ethtool_self_test(dev, useraddr);
		break;
	case ETHTOOL_GSTRINGS:
		rc = ethtool_get_strings(dev, useraddr);
		break;
	case ETHTOOL_PHYS_ID:
		rc = ethtool_phys_id(dev, useraddr);
		break;
	case ETHTOOL_GSTATS:
		rc = ethtool_get_stats(dev, useraddr);
		break;
	case ETHTOOL_GPERMADDR:
		rc = ethtool_get_perm_addr(dev, useraddr);
		break;
	default:
		rc =  -EOPNOTSUPP;
	}
	
	if(dev->ethtool_ops->complete)
		dev->ethtool_ops->complete(dev);

	if (old_features != dev->features)
		netdev_features_change(dev);

	return rc;

 ioctl:
	if (dev->do_ioctl)
		return dev->do_ioctl(dev, ifr, SIOCETHTOOL);
	return -EOPNOTSUPP;
}