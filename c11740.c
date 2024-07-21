static ssize_t status_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	char *status;

	switch (to_devlink(dev)->status) {
	case DL_STATE_NONE:
		status = "not tracked"; break;
	case DL_STATE_DORMANT:
		status = "dormant"; break;
	case DL_STATE_AVAILABLE:
		status = "available"; break;
	case DL_STATE_CONSUMER_PROBE:
		status = "consumer probing"; break;
	case DL_STATE_ACTIVE:
		status = "active"; break;
	case DL_STATE_SUPPLIER_UNBIND:
		status = "supplier unbinding"; break;
	default:
		status = "unknown"; break;
	}
	return sprintf(buf, "%s\n", status);
}