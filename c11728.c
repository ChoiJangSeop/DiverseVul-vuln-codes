ssize_t __weak cpu_show_tsx_async_abort(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "Not affected\n");
}