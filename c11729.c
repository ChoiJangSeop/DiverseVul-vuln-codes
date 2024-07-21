static ssize_t write_policy_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct cacheinfo *this_leaf = dev_get_drvdata(dev);
	unsigned int ci_attr = this_leaf->attributes;
	int n = 0;

	if (ci_attr & CACHE_WRITE_THROUGH)
		n = sprintf(buf, "WriteThrough\n");
	else if (ci_attr & CACHE_WRITE_BACK)
		n = sprintf(buf, "WriteBack\n");
	return n;
}