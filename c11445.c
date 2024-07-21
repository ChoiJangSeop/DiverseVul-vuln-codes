int nfc_register_device(struct nfc_dev *dev)
{
	int rc;

	pr_debug("dev_name=%s\n", dev_name(&dev->dev));

	mutex_lock(&nfc_devlist_mutex);
	nfc_devlist_generation++;
	rc = device_add(&dev->dev);
	mutex_unlock(&nfc_devlist_mutex);

	if (rc < 0)
		return rc;

	rc = nfc_llcp_register_device(dev);
	if (rc)
		pr_err("Could not register llcp device\n");

	device_lock(&dev->dev);
	dev->rfkill = rfkill_alloc(dev_name(&dev->dev), &dev->dev,
				   RFKILL_TYPE_NFC, &nfc_rfkill_ops, dev);
	if (dev->rfkill) {
		if (rfkill_register(dev->rfkill) < 0) {
			rfkill_destroy(dev->rfkill);
			dev->rfkill = NULL;
		}
	}
	device_unlock(&dev->dev);

	rc = nfc_genl_device_added(dev);
	if (rc)
		pr_debug("The userspace won't be notified that the device %s was added\n",
			 dev_name(&dev->dev));

	return 0;
}