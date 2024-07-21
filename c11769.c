static ssize_t runtime_enabled_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	if (dev->power.disable_depth && (dev->power.runtime_auto == false))
		return sprintf(buf, "disabled & forbidden\n");
	if (dev->power.disable_depth)
		return sprintf(buf, "disabled\n");
	if (dev->power.runtime_auto == false)
		return sprintf(buf, "forbidden\n");
	return sprintf(buf, "enabled\n");
}