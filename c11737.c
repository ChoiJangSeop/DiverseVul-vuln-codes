static ssize_t print_cpus_nohz_full(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%*pbl\n", cpumask_pr_args(tick_nohz_full_mask));
}