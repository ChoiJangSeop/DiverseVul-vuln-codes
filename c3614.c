do_add_counters(struct net *net, const void __user *user, unsigned int len,
		int compat)
{
	unsigned int i;
	struct xt_counters_info tmp;
	struct xt_counters *paddc;
	unsigned int num_counters;
	char *name;
	int size;
	void *ptmp;
	struct xt_table *t;
	const struct xt_table_info *private;
	int ret = 0;
	struct ip6t_entry *iter;
	unsigned int addend;
#ifdef CONFIG_COMPAT
	struct compat_xt_counters_info compat_tmp;

	if (compat) {
		ptmp = &compat_tmp;
		size = sizeof(struct compat_xt_counters_info);
	} else
#endif
	{
		ptmp = &tmp;
		size = sizeof(struct xt_counters_info);
	}

	if (copy_from_user(ptmp, user, size) != 0)
		return -EFAULT;

#ifdef CONFIG_COMPAT
	if (compat) {
		num_counters = compat_tmp.num_counters;
		name = compat_tmp.name;
	} else
#endif
	{
		num_counters = tmp.num_counters;
		name = tmp.name;
	}

	if (len != size + num_counters * sizeof(struct xt_counters))
		return -EINVAL;

	paddc = vmalloc(len - size);
	if (!paddc)
		return -ENOMEM;

	if (copy_from_user(paddc, user + size, len - size) != 0) {
		ret = -EFAULT;
		goto free;
	}

	t = xt_find_table_lock(net, AF_INET6, name);
	if (IS_ERR_OR_NULL(t)) {
		ret = t ? PTR_ERR(t) : -ENOENT;
		goto free;
	}

	local_bh_disable();
	private = t->private;
	if (private->number != num_counters) {
		ret = -EINVAL;
		goto unlock_up_free;
	}

	i = 0;
	addend = xt_write_recseq_begin();
	xt_entry_foreach(iter, private->entries, private->size) {
		struct xt_counters *tmp;

		tmp = xt_get_this_cpu_counter(&iter->counters);
		ADD_COUNTER(*tmp, paddc[i].bcnt, paddc[i].pcnt);
		++i;
	}
	xt_write_recseq_end(addend);
 unlock_up_free:
	local_bh_enable();
	xt_table_unlock(t);
	module_put(t->me);
 free:
	vfree(paddc);

	return ret;
}