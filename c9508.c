static int hugetlb_sysctl_handler_common(bool obey_mempolicy,
			 struct ctl_table *table, int write,
			 void *buffer, size_t *length, loff_t *ppos)
{
	struct hstate *h = &default_hstate;
	unsigned long tmp = h->max_huge_pages;
	int ret;

	if (!hugepages_supported())
		return -EOPNOTSUPP;

	table->data = &tmp;
	table->maxlen = sizeof(unsigned long);
	ret = proc_doulongvec_minmax(table, write, buffer, length, ppos);
	if (ret)
		goto out;

	if (write)
		ret = __nr_hugepages_store_common(obey_mempolicy, h,
						  NUMA_NO_NODE, tmp, *length);
out:
	return ret;
}