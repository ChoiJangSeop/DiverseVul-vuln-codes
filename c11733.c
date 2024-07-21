ssize_t __weak cpu_show_itlb_multihit(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Not affected\n");
}