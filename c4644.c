static ssize_t environ_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	char *page;
	unsigned long src = *ppos;
	int ret = 0;
	struct mm_struct *mm = file->private_data;
	unsigned long env_start, env_end;

	if (!mm)
		return 0;

	page = (char *)__get_free_page(GFP_TEMPORARY);
	if (!page)
		return -ENOMEM;

	ret = 0;
	if (!atomic_inc_not_zero(&mm->mm_users))
		goto free;

	down_read(&mm->mmap_sem);
	env_start = mm->env_start;
	env_end = mm->env_end;
	up_read(&mm->mmap_sem);

	while (count > 0) {
		size_t this_len, max_len;
		int retval;

		if (src >= (env_end - env_start))
			break;

		this_len = env_end - (env_start + src);

		max_len = min_t(size_t, PAGE_SIZE, count);
		this_len = min(max_len, this_len);

		retval = access_remote_vm(mm, (env_start + src),
			page, this_len, 0);

		if (retval <= 0) {
			ret = retval;
			break;
		}

		if (copy_to_user(buf, page, retval)) {
			ret = -EFAULT;
			break;
		}

		ret += retval;
		src += retval;
		buf += retval;
		count -= retval;
	}
	*ppos = src;
	mmput(mm);

free:
	free_page((unsigned long) page);
	return ret;
}