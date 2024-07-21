static ssize_t debug_read_tlb(struct file *file, char __user *userbuf,
			      size_t count, loff_t *ppos)
{
	struct omap_iommu *obj = file->private_data;
	char *p, *buf;
	ssize_t bytes, rest;

	if (is_omap_iommu_detached(obj))
		return -EPERM;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	p = buf;

	mutex_lock(&iommu_debug_lock);

	p += sprintf(p, "%8s %8s\n", "cam:", "ram:");
	p += sprintf(p, "-----------------------------------------\n");
	rest = count - (p - buf);
	p += omap_dump_tlb_entries(obj, p, rest);

	bytes = simple_read_from_buffer(userbuf, count, ppos, buf, p - buf);

	mutex_unlock(&iommu_debug_lock);
	kfree(buf);

	return bytes;
}