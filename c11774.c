ssize_t device_show_ulong(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	struct dev_ext_attribute *ea = to_ext_attr(attr);
	return snprintf(buf, PAGE_SIZE, "%lx\n", *(unsigned long *)(ea->var));
}