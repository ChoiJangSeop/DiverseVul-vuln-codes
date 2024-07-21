static void redo_fd_request(void)
{
	int drive;
	int tmp;

	lastredo = jiffies;
	if (current_drive < N_DRIVE)
		floppy_off(current_drive);

do_request:
	if (!current_req) {
		int pending;

		spin_lock_irq(&floppy_lock);
		pending = set_next_request();
		spin_unlock_irq(&floppy_lock);
		if (!pending) {
			do_floppy = NULL;
			unlock_fdc();
			return;
		}
	}
	drive = (long)current_req->q->disk->private_data;
	set_fdc(drive);
	reschedule_timeout(current_drive, "redo fd request");

	set_floppy(drive);
	raw_cmd = &default_raw_cmd;
	raw_cmd->flags = 0;
	if (start_motor(redo_fd_request))
		return;

	disk_change(current_drive);
	if (test_bit(current_drive, &fake_change) ||
	    test_bit(FD_DISK_CHANGED_BIT, &drive_state[current_drive].flags)) {
		DPRINT("disk absent or changed during operation\n");
		request_done(0);
		goto do_request;
	}
	if (!_floppy) {	/* Autodetection */
		if (!probing) {
			drive_state[current_drive].probed_format = 0;
			if (next_valid_format(current_drive)) {
				DPRINT("no autodetectable formats\n");
				_floppy = NULL;
				request_done(0);
				goto do_request;
			}
		}
		probing = 1;
		_floppy = floppy_type + drive_params[current_drive].autodetect[drive_state[current_drive].probed_format];
	} else
		probing = 0;
	errors = &(current_req->error_count);
	tmp = make_raw_rw_request();
	if (tmp < 2) {
		request_done(tmp);
		goto do_request;
	}

	if (test_bit(FD_NEED_TWADDLE_BIT, &drive_state[current_drive].flags))
		twaddle(current_fdc, current_drive);
	schedule_bh(floppy_start);
	debugt(__func__, "queue fd request");
	return;
}