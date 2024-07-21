static void k_shift(struct vc_data *vc, unsigned char value, char up_flag)
{
	int old_state = shift_state;

	if (rep)
		return;
	/*
	 * Mimic typewriter:
	 * a CapsShift key acts like Shift but undoes CapsLock
	 */
	if (value == KVAL(K_CAPSSHIFT)) {
		value = KVAL(K_SHIFT);
		if (!up_flag)
			clr_vc_kbd_led(kbd, VC_CAPSLOCK);
	}

	if (up_flag) {
		/*
		 * handle the case that two shift or control
		 * keys are depressed simultaneously
		 */
		if (shift_down[value])
			shift_down[value]--;
	} else
		shift_down[value]++;

	if (shift_down[value])
		shift_state |= (1 << value);
	else
		shift_state &= ~(1 << value);

	/* kludge */
	if (up_flag && shift_state != old_state && npadch != -1) {
		if (kbd->kbdmode == VC_UNICODE)
			to_utf8(vc, npadch);
		else
			put_queue(vc, npadch & 0xff);
		npadch = -1;
	}
}