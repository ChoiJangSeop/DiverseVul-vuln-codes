static ssize_t auto_remove_on_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct device_link *link = to_devlink(dev);
	char *str;

	if (link->flags & DL_FLAG_AUTOREMOVE_SUPPLIER)
		str = "supplier unbind";
	else if (link->flags & DL_FLAG_AUTOREMOVE_CONSUMER)
		str = "consumer unbind";
	else
		str = "never";

	return sprintf(buf, "%s\n", str);
}