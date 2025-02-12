static int do_replace(struct net *net, const void __user *user,
		      unsigned int len)
{
	int ret, countersize;
	struct ebt_table_info *newinfo;
	struct ebt_replace tmp;

	if (copy_from_user(&tmp, user, sizeof(tmp)) != 0)
		return -EFAULT;

	if (len != sizeof(tmp) + tmp.entries_size) {
		BUGPRINT("Wrong len argument\n");
		return -EINVAL;
	}

	if (tmp.entries_size == 0) {
		BUGPRINT("Entries_size never zero\n");
		return -EINVAL;
	}
	/* overflow check */
	if (tmp.nentries >= ((INT_MAX - sizeof(struct ebt_table_info)) /
			NR_CPUS - SMP_CACHE_BYTES) / sizeof(struct ebt_counter))
		return -ENOMEM;
	if (tmp.num_counters >= INT_MAX / sizeof(struct ebt_counter))
		return -ENOMEM;

	countersize = COUNTER_OFFSET(tmp.nentries) * nr_cpu_ids;
	newinfo = vmalloc(sizeof(*newinfo) + countersize);
	if (!newinfo)
		return -ENOMEM;

	if (countersize)
		memset(newinfo->counters, 0, countersize);

	newinfo->entries = vmalloc(tmp.entries_size);
	if (!newinfo->entries) {
		ret = -ENOMEM;
		goto free_newinfo;
	}
	if (copy_from_user(
	   newinfo->entries, tmp.entries, tmp.entries_size) != 0) {
		BUGPRINT("Couldn't copy entries from userspace\n");
		ret = -EFAULT;
		goto free_entries;
	}

	ret = do_replace_finish(net, &tmp, newinfo);
	if (ret == 0)
		return ret;
free_entries:
	vfree(newinfo->entries);
free_newinfo:
	vfree(newinfo);
	return ret;
}