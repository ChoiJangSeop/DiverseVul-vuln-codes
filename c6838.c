input_init(struct window_pane *wp)
{
	struct input_ctx	*ictx;

	ictx = wp->ictx = xcalloc(1, sizeof *ictx);

	ictx->input_space = INPUT_BUF_START;
	ictx->input_buf = xmalloc(INPUT_BUF_START);

	ictx->since_ground = evbuffer_new();

	evtimer_set(&ictx->timer, input_timer_callback, ictx);

	input_reset(wp, 0);
}