struct mapped_device *dm_get_from_kobject(struct kobject *kobj)
{
	struct mapped_device *md;

	md = container_of(kobj, struct mapped_device, kobj_holder.kobj);

	if (test_bit(DMF_FREEING, &md->flags) ||
	    dm_deleting_md(md))
		return NULL;

	dm_get(md);
	return md;
}