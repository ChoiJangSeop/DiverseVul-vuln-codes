static ssize_t name_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct wakeup_source *ws = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", ws->name);
}