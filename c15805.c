static void bad_flp_intr(void)
{
	int err_count;

	if (probing) {
		drive_state[current_drive].probed_format++;
		if (!next_valid_format(current_drive))
			return;
	}
	err_count = ++(*errors);
	INFBOUND(write_errors[current_drive].badness, err_count);
	if (err_count > drive_params[current_drive].max_errors.abort)
		cont->done(0);
	if (err_count > drive_params[current_drive].max_errors.reset)
		fdc_state[current_fdc].reset = 1;
	else if (err_count > drive_params[current_drive].max_errors.recal)
		drive_state[current_drive].track = NEED_2_RECAL;
}