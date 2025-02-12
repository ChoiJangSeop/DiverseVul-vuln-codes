void nfc_unregister_device(struct nfc_dev *dev)
{
	int rc;

	pr_debug("dev_name=%s\n", dev_name(&dev->dev));

	rc = nfc_genl_device_removed(dev);
	if (rc)
		pr_debug("The userspace won't be notified that the device %s "
			 "was removed\n", dev_name(&dev->dev));

	device_lock(&dev->dev);
	if (dev->rfkill) {
		rfkill_unregister(dev->rfkill);
		rfkill_destroy(dev->rfkill);
	}
	device_unlock(&dev->dev);

	if (dev->ops->check_presence) {
		device_lock(&dev->dev);
		dev->shutting_down = true;
		device_unlock(&dev->dev);
		del_timer_sync(&dev->check_pres_timer);
		cancel_work_sync(&dev->check_pres_work);
	}

	nfc_llcp_unregister_device(dev);

	mutex_lock(&nfc_devlist_mutex);
	nfc_devlist_generation++;
	device_del(&dev->dev);
	mutex_unlock(&nfc_devlist_mutex);
}