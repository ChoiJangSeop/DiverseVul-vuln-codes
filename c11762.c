static ssize_t state_synced_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	bool val;

	device_lock(dev);
	val = dev->state_synced;
	device_unlock(dev);
	return sprintf(buf, "%u\n", val);
}