static int interpret_errors(void)
{
	char bad;

	if (inr != 7) {
		DPRINT("-- FDC reply error\n");
		fdc_state[current_fdc].reset = 1;
		return 1;
	}

	/* check IC to find cause of interrupt */
	switch (reply_buffer[ST0] & ST0_INTR) {
	case 0x40:		/* error occurred during command execution */
		if (reply_buffer[ST1] & ST1_EOC)
			return 0;	/* occurs with pseudo-DMA */
		bad = 1;
		if (reply_buffer[ST1] & ST1_WP) {
			DPRINT("Drive is write protected\n");
			clear_bit(FD_DISK_WRITABLE_BIT,
				  &drive_state[current_drive].flags);
			cont->done(0);
			bad = 2;
		} else if (reply_buffer[ST1] & ST1_ND) {
			set_bit(FD_NEED_TWADDLE_BIT,
				&drive_state[current_drive].flags);
		} else if (reply_buffer[ST1] & ST1_OR) {
			if (drive_params[current_drive].flags & FTD_MSG)
				DPRINT("Over/Underrun - retrying\n");
			bad = 0;
		} else if (*errors >= drive_params[current_drive].max_errors.reporting) {
			print_errors();
		}
		if (reply_buffer[ST2] & ST2_WC || reply_buffer[ST2] & ST2_BC)
			/* wrong cylinder => recal */
			drive_state[current_drive].track = NEED_2_RECAL;
		return bad;
	case 0x80:		/* invalid command given */
		DPRINT("Invalid FDC command given!\n");
		cont->done(0);
		return 2;
	case 0xc0:
		DPRINT("Abnormal termination caused by polling\n");
		cont->error();
		return 2;
	default:		/* (0) Normal command termination */
		return 0;
	}
}