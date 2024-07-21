static ssize_t pm_qos_no_power_off_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", !!(dev_pm_qos_requested_flags(dev)
					& PM_QOS_FLAG_NO_POWER_OFF));
}