int nfc_dep_link_down(struct nfc_dev *dev)
{
	int rc = 0;

	pr_debug("dev_name=%s\n", dev_name(&dev->dev));

	if (!dev->ops->dep_link_down)
		return -EOPNOTSUPP;

	device_lock(&dev->dev);

	if (!device_is_registered(&dev->dev)) {
		rc = -ENODEV;
		goto error;
	}

	if (dev->dep_link_up == false) {
		rc = -EALREADY;
		goto error;
	}

	rc = dev->ops->dep_link_down(dev);
	if (!rc) {
		dev->dep_link_up = false;
		dev->active_target = NULL;
		dev->rf_mode = NFC_RF_NONE;
		nfc_llcp_mac_is_down(dev);
		nfc_genl_dep_link_down_event(dev);
	}

error:
	device_unlock(&dev->dev);

	return rc;
}