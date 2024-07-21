ssize_t __weak cpu_show_l1tf(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Not affected\n");
}