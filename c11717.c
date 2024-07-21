static ssize_t waiting_for_supplier_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	bool val;

	device_lock(dev);
	mutex_lock(&wfs_lock);
	val = !list_empty(&dev->links.needs_suppliers)
	      && dev->links.need_for_probe;
	mutex_unlock(&wfs_lock);
	device_unlock(dev);
	return sprintf(buf, "%u\n", val);
}