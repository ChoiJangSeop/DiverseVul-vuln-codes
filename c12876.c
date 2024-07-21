static ssize_t available_instances_show(struct mdev_type *mtype,
					struct mdev_type_attribute *attr,
					char *buf)
{
	const struct mbochs_type *type =
		&mbochs_types[mtype_get_type_group_id(mtype)];
	int count = (max_mbytes - mbochs_used_mbytes) / type->mbytes;

	return sprintf(buf, "%d\n", count);
}