static ssize_t wakeup_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "%s\n", device_can_wakeup(dev)
		? (device_may_wakeup(dev) ? _enabled : _disabled)
		: "");
}