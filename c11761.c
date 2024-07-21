static ssize_t runtime_usage_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&dev->power.usage_count));
}