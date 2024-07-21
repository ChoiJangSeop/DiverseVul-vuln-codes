static ssize_t print_cpus_isolated(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int n;
	cpumask_var_t isolated;

	if (!alloc_cpumask_var(&isolated, GFP_KERNEL))
		return -ENOMEM;

	cpumask_andnot(isolated, cpu_possible_mask,
		       housekeeping_cpumask(HK_FLAG_DOMAIN));
	n = sprintf(buf, "%*pbl\n", cpumask_pr_args(isolated));

	free_cpumask_var(isolated);

	return n;
}