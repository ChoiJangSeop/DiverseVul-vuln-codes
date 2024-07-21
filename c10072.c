show_ep_handle(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct iscsi_endpoint *ep = iscsi_dev_to_endpoint(dev);
	return sprintf(buf, "%llu\n", (unsigned long long) ep->id);
}