static int fat_ioctl_filldir(void *__buf, const char *name, int name_len,
			     loff_t offset, u64 ino, unsigned int d_type)
{
	struct fat_ioctl_filldir_callback *buf = __buf;
	struct dirent __user *d1 = buf->dirent;
	struct dirent __user *d2 = d1 + 1;

	if (buf->result)
		return -EINVAL;
	buf->result++;

	if (name != NULL) {
		/* dirent has only short name */
		if (name_len >= sizeof(d1->d_name))
			name_len = sizeof(d1->d_name) - 1;

		if (put_user(0, d2->d_name)			||
		    put_user(0, &d2->d_reclen)			||
		    copy_to_user(d1->d_name, name, name_len)	||
		    put_user(0, d1->d_name + name_len)		||
		    put_user(name_len, &d1->d_reclen))
			goto efault;
	} else {
		/* dirent has short and long name */
		const char *longname = buf->longname;
		int long_len = buf->long_len;
		const char *shortname = buf->shortname;
		int short_len = buf->short_len;

		if (long_len >= sizeof(d1->d_name))
			long_len = sizeof(d1->d_name) - 1;
		if (short_len >= sizeof(d1->d_name))
			short_len = sizeof(d1->d_name) - 1;

		if (copy_to_user(d2->d_name, longname, long_len)	||
		    put_user(0, d2->d_name + long_len)			||
		    put_user(long_len, &d2->d_reclen)			||
		    put_user(ino, &d2->d_ino)				||
		    put_user(offset, &d2->d_off)			||
		    copy_to_user(d1->d_name, shortname, short_len)	||
		    put_user(0, d1->d_name + short_len)			||
		    put_user(short_len, &d1->d_reclen))
			goto efault;
	}
	return 0;
efault:
	buf->result = -EFAULT;
	return -EFAULT;
}