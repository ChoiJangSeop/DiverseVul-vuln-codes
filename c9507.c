int hugetlb_overcommit_handler(struct ctl_table *table, int write,
		void *buffer, size_t *length, loff_t *ppos)
{
	struct hstate *h = &default_hstate;
	unsigned long tmp;
	int ret;

	if (!hugepages_supported())
		return -EOPNOTSUPP;

	tmp = h->nr_overcommit_huge_pages;

	if (write && hstate_is_gigantic(h))
		return -EINVAL;

	table->data = &tmp;
	table->maxlen = sizeof(unsigned long);
	ret = proc_doulongvec_minmax(table, write, buffer, length, ppos);
	if (ret)
		goto out;

	if (write) {
		spin_lock(&hugetlb_lock);
		h->nr_overcommit_huge_pages = tmp;
		spin_unlock(&hugetlb_lock);
	}
out:
	return ret;
}