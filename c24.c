static int vfat_ioctl32(unsigned fd, unsigned cmd, unsigned long arg)
{
	struct compat_dirent __user *p = compat_ptr(arg);
	int ret;
	mm_segment_t oldfs = get_fs();
	struct dirent d[2];

	switch(cmd)
	{
        	case VFAT_IOCTL_READDIR_BOTH32:
                	cmd = VFAT_IOCTL_READDIR_BOTH;
                	break;
        	case VFAT_IOCTL_READDIR_SHORT32:
                	cmd = VFAT_IOCTL_READDIR_SHORT;
                	break;
	}

	set_fs(KERNEL_DS);
	ret = sys_ioctl(fd,cmd,(unsigned long)&d);
	set_fs(oldfs);
	if (ret >= 0) {
		ret |= put_dirent32(&d[0], p);
		ret |= put_dirent32(&d[1], p + 1);
	}
	return ret;
}