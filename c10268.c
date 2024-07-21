static int finish(void)
{
	int magic = BPF_PRELOAD_END;
	struct pid *tgid;
	loff_t pos = 0;
	ssize_t n;

	/* send the last magic to UMD. It will do a normal exit. */
	n = kernel_write(umd_ops.info.pipe_to_umh,
			 &magic, sizeof(magic), &pos);
	if (n != sizeof(magic))
		return -EPIPE;
	tgid = umd_ops.info.tgid;
	wait_event(tgid->wait_pidfd, thread_group_exited(tgid));
	umd_ops.info.tgid = NULL;
	return 0;
}