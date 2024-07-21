static int do_siocgstampns(struct net *net, struct socket *sock,
			 unsigned int cmd, struct compat_timespec __user *up)
{
	mm_segment_t old_fs = get_fs();
	struct timespec kts;
	int err;

	set_fs(KERNEL_DS);
	err = sock_do_ioctl(net, sock, cmd, (unsigned long)&kts);
	set_fs(old_fs);
	if (!err) {
		err = put_user(kts.tv_sec, &up->tv_sec);
		err |= __put_user(kts.tv_nsec, &up->tv_nsec);
	}
	return err;
}