static ssize_t runtime_active_kids_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", dev->power.ignore_children ?
		0 : atomic_read(&dev->power.child_count));
}