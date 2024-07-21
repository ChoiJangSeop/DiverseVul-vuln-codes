static ssize_t phys_index_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct memory_block *mem = to_memory_block(dev);
	unsigned long phys_index;

	phys_index = mem->start_section_nr / sections_per_block;
	return sprintf(buf, "%08lx\n", phys_index);
}