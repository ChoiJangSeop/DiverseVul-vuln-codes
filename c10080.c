show_transport_handle(struct device *dev, struct device_attribute *attr,
		      char *buf)
{
	struct iscsi_internal *priv = dev_to_iscsi_internal(dev);
	return sprintf(buf, "%llu\n", (unsigned long long)iscsi_handle(priv->iscsi_transport));
}