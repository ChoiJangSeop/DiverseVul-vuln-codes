static ssize_t numa_node_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", dev_to_node(dev));
}