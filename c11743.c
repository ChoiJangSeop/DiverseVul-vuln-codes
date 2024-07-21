static ssize_t pm_qos_resume_latency_us_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	s32 value = dev_pm_qos_requested_resume_latency(dev);

	if (value == 0)
		return sprintf(buf, "n/a\n");
	if (value == PM_QOS_RESUME_LATENCY_NO_CONSTRAINT)
		value = 0;

	return sprintf(buf, "%d\n", value);
}