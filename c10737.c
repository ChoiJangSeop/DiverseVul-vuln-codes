static int vt_kdsetmode(struct vc_data *vc, unsigned long mode)
{
	switch (mode) {
	case KD_GRAPHICS:
		break;
	case KD_TEXT0:
	case KD_TEXT1:
		mode = KD_TEXT;
		fallthrough;
	case KD_TEXT:
		break;
	default:
		return -EINVAL;
	}

	/* FIXME: this needs the console lock extending */
	if (vc->vc_mode == mode)
		return 0;

	vc->vc_mode = mode;
	if (vc->vc_num != fg_console)
		return 0;

	/* explicitly blank/unblank the screen if switching modes */
	console_lock();
	if (mode == KD_TEXT)
		do_unblank_screen(1);
	else
		do_blank_screen(1);
	console_unlock();

	return 0;
}