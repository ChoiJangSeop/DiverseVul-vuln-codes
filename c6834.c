tty_open(struct tty *tty, char **cause)
{
	tty->term = tty_term_find(tty->term_name, tty->fd, cause);
	if (tty->term == NULL) {
		tty_close(tty);
		return (-1);
	}
	tty->flags |= TTY_OPENED;

	tty->flags &= ~(TTY_NOCURSOR|TTY_FREEZE|TTY_BLOCK|TTY_TIMER);

	event_set(&tty->event_in, tty->fd, EV_PERSIST|EV_READ,
	    tty_read_callback, tty);
	tty->in = evbuffer_new();

	event_set(&tty->event_out, tty->fd, EV_WRITE, tty_write_callback, tty);
	tty->out = evbuffer_new();

	evtimer_set(&tty->timer, tty_timer_callback, tty);

	tty_start_tty(tty);

	tty_keys_build(tty);

	return (0);
}