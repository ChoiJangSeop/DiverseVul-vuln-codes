control_notify_input(struct client *c, struct window_pane *wp,
    struct evbuffer *input)
{
	u_char		*buf;
	size_t		 len;
	struct evbuffer *message;
	u_int		 i;

	if (c->session == NULL)
	    return;

	buf = EVBUFFER_DATA(input);
	len = EVBUFFER_LENGTH(input);

	/*
	 * Only write input if the window pane is linked to a window belonging
	 * to the client's session.
	 */
	if (winlink_find_by_window(&c->session->windows, wp->window) != NULL) {
		message = evbuffer_new();
		evbuffer_add_printf(message, "%%output %%%u ", wp->id);
		for (i = 0; i < len; i++) {
			if (buf[i] < ' ' || buf[i] == '\\')
			    evbuffer_add_printf(message, "\\%03o", buf[i]);
			else
			    evbuffer_add_printf(message, "%c", buf[i]);
		}
		control_write_buffer(c, message);
		evbuffer_free(message);
	}
}