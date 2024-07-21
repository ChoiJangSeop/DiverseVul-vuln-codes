static ssize_t size_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct cacheinfo *this_leaf = dev_get_drvdata(dev);

	return sprintf(buf, "%uK\n", this_leaf->size >> 10);
}