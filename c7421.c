qb_ipcs_create(const char *name,
	       int32_t service_id,
	       enum qb_ipc_type type, struct qb_ipcs_service_handlers *handlers)
{
	struct qb_ipcs_service *s;
	int fd;
	unsigned int seed;

	s = calloc(1, sizeof(struct qb_ipcs_service));
	if (s == NULL) {
		return NULL;
	}
	if (type == QB_IPC_NATIVE) {
#ifdef DISABLE_IPC_SHM
		s->type = QB_IPC_SOCKET;
#else
		s->type = QB_IPC_SHM;
#endif /* DISABLE_IPC_SHM */
	} else {
		s->type = type;
	}

	s->pid = getpid();
	s->needs_sock_for_poll = QB_FALSE;
	s->poll_priority = QB_LOOP_MED;

	/* Initial alloc ref */
	qb_ipcs_ref(s);

	s->service_id = service_id;
	(void)strlcpy(s->name, name, NAME_MAX);

	s->serv_fns.connection_accept = handlers->connection_accept;
	s->serv_fns.connection_created = handlers->connection_created;
	s->serv_fns.msg_process = handlers->msg_process;
	s->serv_fns.connection_closed = handlers->connection_closed;
	s->serv_fns.connection_destroyed = handlers->connection_destroyed;

	qb_list_init(&s->connections);
	qb_list_init(&s->list);
	qb_list_add(&s->list, &qb_ipc_services);

	/* Randomise socket names */
	fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1) {
		seed = (time_t)time(NULL);
	} else {
		if (read(fd, &seed, sizeof(seed)) != 4) {
			seed = (time_t)time(NULL);
		}
		close(fd);
	}
	srand(seed);

	return s;
}