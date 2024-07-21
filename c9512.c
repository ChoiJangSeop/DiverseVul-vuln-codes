static ssize_t rbd_config_info_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct rbd_device *rbd_dev = dev_to_rbd_dev(dev);

	return sprintf(buf, "%s\n", rbd_dev->config_info);
}