static int get_floppy_geometry(int drive, int type, struct floppy_struct **g)
{
	if (type)
		*g = &floppy_type[type];
	else {
		if (lock_fdc(drive, false))
			return -EINTR;
		if (poll_drive(false, 0) == -EINTR)
			return -EINTR;
		process_fd_request();
		*g = current_type[drive];
	}
	if (!*g)
		return -ENODEV;
	return 0;
}