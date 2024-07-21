static long fat_compat_dir_ioctl(struct file *file, unsigned cmd,
				 unsigned long arg)
{
	struct compat_dirent __user *p = compat_ptr(arg);
	int ret;
	mm_segment_t oldfs = get_fs();
	struct dirent d[2];

	switch (cmd) {
	case VFAT_IOCTL_READDIR_BOTH32:
		cmd = VFAT_IOCTL_READDIR_BOTH;
		break;
	case VFAT_IOCTL_READDIR_SHORT32:
		cmd = VFAT_IOCTL_READDIR_SHORT;
		break;
	default:
		return -ENOIOCTLCMD;
	}

	set_fs(KERNEL_DS);
	lock_kernel();
	ret = fat_dir_ioctl(file->f_path.dentry->d_inode, file,
			    cmd, (unsigned long) &d);
	unlock_kernel();
	set_fs(oldfs);
	if (ret >= 0) {
		ret |= fat_compat_put_dirent32(&d[0], p);
		ret |= fat_compat_put_dirent32(&d[1], p + 1);
	}
	return ret;
}