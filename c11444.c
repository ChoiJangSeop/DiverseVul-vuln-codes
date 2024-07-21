int nfc_enable_se(struct nfc_dev *dev, u32 se_idx)
{
	struct nfc_se *se;
	int rc;

	pr_debug("%s se index %d\n", dev_name(&dev->dev), se_idx);

	device_lock(&dev->dev);

	if (!device_is_registered(&dev->dev)) {
		rc = -ENODEV;
		goto error;
	}

	if (!dev->dev_up) {
		rc = -ENODEV;
		goto error;
	}

	if (dev->polling) {
		rc = -EBUSY;
		goto error;
	}

	if (!dev->ops->enable_se || !dev->ops->disable_se) {
		rc = -EOPNOTSUPP;
		goto error;
	}

	se = nfc_find_se(dev, se_idx);
	if (!se) {
		rc = -EINVAL;
		goto error;
	}

	if (se->state == NFC_SE_ENABLED) {
		rc = -EALREADY;
		goto error;
	}

	rc = dev->ops->enable_se(dev, se_idx);
	if (rc >= 0)
		se->state = NFC_SE_ENABLED;

error:
	device_unlock(&dev->dev);
	return rc;
}