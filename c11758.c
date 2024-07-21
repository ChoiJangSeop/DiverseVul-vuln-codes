static ssize_t sync_state_only_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct device_link *link = to_devlink(dev);

	return sprintf(buf, "%d\n", !!(link->flags & DL_FLAG_SYNC_STATE_ONLY));
}