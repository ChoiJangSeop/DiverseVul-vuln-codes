static ssize_t runtime_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char *p;

	if (dev->power.runtime_error) {
		p = "error\n";
	} else if (dev->power.disable_depth) {
		p = "unsupported\n";
	} else {
		switch (dev->power.runtime_status) {
		case RPM_SUSPENDED:
			p = "suspended\n";
			break;
		case RPM_SUSPENDING:
			p = "suspending\n";
			break;
		case RPM_RESUMING:
			p = "resuming\n";
			break;
		case RPM_ACTIVE:
			p = "active\n";
			break;
		default:
			return -EIO;
		}
	}
	return sprintf(buf, p);
}