static ssize_t show_crash_notes(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct cpu *cpu = container_of(dev, struct cpu, dev);
	ssize_t rc;
	unsigned long long addr;
	int cpunum;

	cpunum = cpu->dev.id;

	/*
	 * Might be reading other cpu's data based on which cpu read thread
	 * has been scheduled. But cpu data (memory) is allocated once during
	 * boot up and this data does not change there after. Hence this
	 * operation should be safe. No locking required.
	 */
	addr = per_cpu_ptr_to_phys(per_cpu_ptr(crash_notes, cpunum));
	rc = sprintf(buf, "%Lx\n", addr);
	return rc;
}