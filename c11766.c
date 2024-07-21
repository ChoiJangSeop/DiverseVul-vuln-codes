ssize_t __weak cpu_show_spectre_v2(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Not affected\n");
}