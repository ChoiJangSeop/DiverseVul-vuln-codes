static int floppy_revalidate(struct gendisk *disk)
{
	int drive = (long)disk->private_data;
	int cf;
	int res = 0;

	if (test_bit(FD_DISK_CHANGED_BIT, &UDRS->flags) ||
	    test_bit(FD_VERIFY_BIT, &UDRS->flags) ||
	    test_bit(drive, &fake_change) ||
	    drive_no_geom(drive)) {
		if (WARN(atomic_read(&usage_count) == 0,
			 "VFS: revalidate called on non-open device.\n"))
			return -EFAULT;

		lock_fdc(drive, false);
		cf = (test_bit(FD_DISK_CHANGED_BIT, &UDRS->flags) ||
		      test_bit(FD_VERIFY_BIT, &UDRS->flags));
		if (!(cf || test_bit(drive, &fake_change) || drive_no_geom(drive))) {
			process_fd_request();	/*already done by another thread */
			return 0;
		}
		UDRS->maxblock = 0;
		UDRS->maxtrack = 0;
		if (buffer_drive == drive)
			buffer_track = -1;
		clear_bit(drive, &fake_change);
		clear_bit(FD_DISK_CHANGED_BIT, &UDRS->flags);
		if (cf)
			UDRS->generation++;
		if (drive_no_geom(drive)) {
			/* auto-sensing */
			res = __floppy_read_block_0(opened_bdev[drive], drive);
		} else {
			if (cf)
				poll_drive(false, FD_RAW_NEED_DISK);
			process_fd_request();
		}
	}
	set_capacity(disk, floppy_sizes[UDRS->fd_device]);
	return res;
}