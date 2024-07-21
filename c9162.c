static int cmm_timeout_handler(struct ctl_table *ctl, int write,
			       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	char buf[64], *p;
	long nr, seconds;
	unsigned int len;

	if (!*lenp || (*ppos && !write)) {
		*lenp = 0;
		return 0;
	}

	if (write) {
		len = *lenp;
		if (copy_from_user(buf, buffer,
				   len > sizeof(buf) ? sizeof(buf) : len))
			return -EFAULT;
		buf[sizeof(buf) - 1] = '\0';
		cmm_skip_blanks(buf, &p);
		nr = simple_strtoul(p, &p, 0);
		cmm_skip_blanks(p, &p);
		seconds = simple_strtoul(p, &p, 0);
		cmm_set_timeout(nr, seconds);
	} else {
		len = sprintf(buf, "%ld %ld\n",
			      cmm_timeout_pages, cmm_timeout_seconds);
		if (len > *lenp)
			len = *lenp;
		if (copy_to_user(buffer, buf, len))
			return -EFAULT;
	}
	*lenp = len;
	*ppos += len;
	return 0;
}