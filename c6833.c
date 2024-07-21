server_client_create(int fd)
{
	struct client	*c;

	setblocking(fd, 0);

	c = xcalloc(1, sizeof *c);
	c->references = 1;
	c->peer = proc_add_peer(server_proc, fd, server_client_dispatch, c);

	if (gettimeofday(&c->creation_time, NULL) != 0)
		fatal("gettimeofday failed");
	memcpy(&c->activity_time, &c->creation_time, sizeof c->activity_time);

	c->environ = environ_create();

	c->fd = -1;
	c->cwd = NULL;

	TAILQ_INIT(&c->queue);

	c->stdin_data = evbuffer_new();
	c->stdout_data = evbuffer_new();
	c->stderr_data = evbuffer_new();

	c->tty.fd = -1;
	c->title = NULL;

	c->session = NULL;
	c->last_session = NULL;

	c->tty.sx = 80;
	c->tty.sy = 24;

	screen_init(&c->status.status, c->tty.sx, 1, 0);

	c->message_string = NULL;
	TAILQ_INIT(&c->message_log);

	c->prompt_string = NULL;
	c->prompt_buffer = NULL;
	c->prompt_index = 0;

	c->flags |= CLIENT_FOCUSED;

	c->keytable = key_bindings_get_table("root", 1);
	c->keytable->references++;

	evtimer_set(&c->repeat_timer, server_client_repeat_timer, c);
	evtimer_set(&c->click_timer, server_client_click_timer, c);

	TAILQ_INSERT_TAIL(&clients, c, entry);
	log_debug("new client %p", c);
	return (c);
}