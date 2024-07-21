static ssize_t runtime_suspended_time_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	u64 tmp = pm_runtime_suspended_time(dev);
	do_div(tmp, NSEC_PER_MSEC);
	ret = sprintf(buf, "%llu\n", tmp);
	return ret;
}