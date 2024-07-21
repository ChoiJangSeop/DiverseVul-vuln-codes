static ssize_t async_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%s\n",
			device_async_suspend_enabled(dev) ?
				_enabled : _disabled);
}