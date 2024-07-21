static int fat_dir_ioctl(struct inode * inode, struct file * filp,
		  unsigned int cmd, unsigned long arg)
{
	struct fat_ioctl_filldir_callback buf;
	struct dirent __user *d1;
	int ret, short_only, both;

	switch (cmd) {
	case VFAT_IOCTL_READDIR_SHORT:
		short_only = 1;
		both = 0;
		break;
	case VFAT_IOCTL_READDIR_BOTH:
		short_only = 0;
		both = 1;
		break;
	default:
		return fat_generic_ioctl(inode, filp, cmd, arg);
	}

	d1 = (struct dirent __user *)arg;
	if (!access_ok(VERIFY_WRITE, d1, sizeof(struct dirent[2])))
		return -EFAULT;
	/*
	 * Yes, we don't need this put_user() absolutely. However old
	 * code didn't return the right value. So, app use this value,
	 * in order to check whether it is EOF.
	 */
	if (put_user(0, &d1->d_reclen))
		return -EFAULT;

	buf.dirent = d1;
	buf.result = 0;
	mutex_lock(&inode->i_mutex);
	ret = -ENOENT;
	if (!IS_DEADDIR(inode)) {
		ret = __fat_readdir(inode, filp, &buf, fat_ioctl_filldir,
				    short_only, both);
	}
	mutex_unlock(&inode->i_mutex);
	if (ret >= 0)
		ret = buf.result;
	return ret;
}