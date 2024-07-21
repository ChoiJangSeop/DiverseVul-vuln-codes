static int __ethtool_set_sg(struct net_device *dev, u32 data)
{
	int err;

	if (!data && dev->ethtool_ops->set_tso) {
		err = dev->ethtool_ops->set_tso(dev, 0);
		if (err)
			return err;
	}

	return dev->ethtool_ops->set_sg(dev, data);
}