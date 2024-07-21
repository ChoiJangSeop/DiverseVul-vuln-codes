static ssize_t soc_info_get(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	struct soc_device *soc_dev = container_of(dev, struct soc_device, dev);

	if (attr == &dev_attr_machine)
		return sprintf(buf, "%s\n", soc_dev->attr->machine);
	if (attr == &dev_attr_family)
		return sprintf(buf, "%s\n", soc_dev->attr->family);
	if (attr == &dev_attr_revision)
		return sprintf(buf, "%s\n", soc_dev->attr->revision);
	if (attr == &dev_attr_serial_number)
		return sprintf(buf, "%s\n", soc_dev->attr->serial_number);
	if (attr == &dev_attr_soc_id)
		return sprintf(buf, "%s\n", soc_dev->attr->soc_id);

	return -EINVAL;

}