static ssize_t removable_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	return sprintf(buf, "%d\n", (int)IS_ENABLED(CONFIG_MEMORY_HOTREMOVE));
}