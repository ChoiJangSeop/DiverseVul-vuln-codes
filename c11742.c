static ssize_t auto_online_blocks_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n",
		       online_type_to_str[memhp_default_online_type]);
}