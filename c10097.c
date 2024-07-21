static int lock_fdc(int drive, bool interruptible)
{
	if (WARN(atomic_read(&usage_count) == 0,
		 "Trying to lock fdc while usage count=0\n"))
		return -1;

	if (wait_event_interruptible(fdc_wait, !test_and_set_bit(0, &fdc_busy)))
		return -EINTR;

	command_status = FD_COMMAND_NONE;

	reschedule_timeout(drive, "lock fdc");
	set_fdc(drive);
	return 0;
}