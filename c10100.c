static int user_reset_fdc(int drive, int arg, bool interruptible)
{
	int ret;

	if (lock_fdc(drive, interruptible))
		return -EINTR;

	if (arg == FD_RESET_ALWAYS)
		FDCS->reset = 1;
	if (FDCS->reset) {
		cont = &reset_cont;
		ret = wait_til_done(reset_fdc, interruptible);
		if (ret == -EINTR)
			return -EINTR;
	}
	process_fd_request();
	return 0;
}