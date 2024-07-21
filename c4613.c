asmlinkage long sys_oabi_fcntl64(unsigned int fd, unsigned int cmd,
				 unsigned long arg)
{
	struct oabi_flock64 user;
	struct flock64 kernel;
	mm_segment_t fs = USER_DS; /* initialized to kill a warning */
	unsigned long local_arg = arg;
	int ret;

	switch (cmd) {
	case F_OFD_GETLK:
	case F_OFD_SETLK:
	case F_OFD_SETLKW:
	case F_GETLK64:
	case F_SETLK64:
	case F_SETLKW64:
		if (copy_from_user(&user, (struct oabi_flock64 __user *)arg,
				   sizeof(user)))
			return -EFAULT;
		kernel.l_type	= user.l_type;
		kernel.l_whence	= user.l_whence;
		kernel.l_start	= user.l_start;
		kernel.l_len	= user.l_len;
		kernel.l_pid	= user.l_pid;
		local_arg = (unsigned long)&kernel;
		fs = get_fs();
		set_fs(KERNEL_DS);
	}

	ret = sys_fcntl64(fd, cmd, local_arg);

	switch (cmd) {
	case F_GETLK64:
		if (!ret) {
			user.l_type	= kernel.l_type;
			user.l_whence	= kernel.l_whence;
			user.l_start	= kernel.l_start;
			user.l_len	= kernel.l_len;
			user.l_pid	= kernel.l_pid;
			if (copy_to_user((struct oabi_flock64 __user *)arg,
					 &user, sizeof(user)))
				ret = -EFAULT;
		}
	case F_SETLK64:
	case F_SETLKW64:
		set_fs(fs);
	}

	return ret;
}