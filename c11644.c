static int con_install(struct tty_driver *driver, struct tty_struct *tty)
{
	unsigned int currcons = tty->index;
	struct vc_data *vc;
	int ret;

	console_lock();
	ret = vc_allocate(currcons);
	if (ret)
		goto unlock;

	vc = vc_cons[currcons].d;

	/* Still being freed */
	if (vc->port.tty) {
		ret = -ERESTARTSYS;
		goto unlock;
	}

	ret = tty_port_install(&vc->port, driver, tty);
	if (ret)
		goto unlock;

	tty->driver_data = vc;
	vc->port.tty = tty;

	if (!tty->winsize.ws_row && !tty->winsize.ws_col) {
		tty->winsize.ws_row = vc_cons[currcons].d->vc_rows;
		tty->winsize.ws_col = vc_cons[currcons].d->vc_cols;
	}
	if (vc->vc_utf)
		tty->termios.c_iflag |= IUTF8;
	else
		tty->termios.c_iflag &= ~IUTF8;
unlock:
	console_unlock();
	return ret;
}