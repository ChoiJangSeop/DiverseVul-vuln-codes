qb_ipcs_us_connect(struct qb_ipcs_service *s,
		   struct qb_ipcs_connection *c,
		   struct qb_ipc_connection_response *r)
{
	char path[PATH_MAX];
	int32_t fd_hdr;
	int32_t res = 0;
	struct ipc_us_control *ctl;
	char *shm_ptr;

	qb_util_log(LOG_DEBUG, "connecting to client (%s)", c->description);

	c->request.u.us.sock = c->setup.u.us.sock;
	c->response.u.us.sock = c->setup.u.us.sock;

	snprintf(r->request, NAME_MAX, "qb-%s-control-%s",
		 s->name, c->description);
	snprintf(r->response, NAME_MAX, "qb-%s-%s", s->name, c->description);

	fd_hdr = qb_sys_mmap_file_open(path, r->request,
				       SHM_CONTROL_SIZE,
				       O_CREAT | O_TRUNC | O_RDWR);
	if (fd_hdr < 0) {
		res = fd_hdr;
		errno = -fd_hdr;
		qb_util_perror(LOG_ERR, "couldn't create file for mmap (%s)",
			       c->description);
		return res;
	}
	(void)strlcpy(r->request, path, PATH_MAX);
	(void)strlcpy(c->request.u.us.shared_file_name, r->request, NAME_MAX);
	res = chown(r->request, c->auth.uid, c->auth.gid);
	if (res != 0) {
		/* ignore res, this is just for the compiler warnings.
		 */
		res = 0;
	}
	res = chmod(r->request, c->auth.mode);
	if (res != 0) {
		/* ignore res, this is just for the compiler warnings.
		 */
		res = 0;
	}

	shm_ptr = mmap(0, SHM_CONTROL_SIZE,
		       PROT_READ | PROT_WRITE, MAP_SHARED, fd_hdr, 0);

	if (shm_ptr == MAP_FAILED) {
		res = -errno;
		qb_util_perror(LOG_ERR, "couldn't create mmap for header (%s)",
			       c->description);
		goto cleanup_hdr;
	}
	c->request.u.us.shared_data = shm_ptr;
	c->response.u.us.shared_data = shm_ptr + sizeof(struct ipc_us_control);
	c->event.u.us.shared_data =  shm_ptr + (2 * sizeof(struct ipc_us_control));

	ctl = (struct ipc_us_control *)c->request.u.us.shared_data;
	ctl->sent = 0;
	ctl->flow_control = 0;
	ctl = (struct ipc_us_control *)c->response.u.us.shared_data;
	ctl->sent = 0;
	ctl->flow_control = 0;
	ctl = (struct ipc_us_control *)c->event.u.us.shared_data;
	ctl->sent = 0;
	ctl->flow_control = 0;

	close(fd_hdr);
	fd_hdr = -1;

	/* request channel */
	res = qb_ipc_dgram_sock_setup(r->response, "request",
				      &c->request.u.us.sock, c->egid);
	if (res < 0) {
		goto cleanup_hdr;
	}

	res = set_sock_size(c->request.u.us.sock, c->request.max_msg_size);
	if (res != 0) {
		goto cleanup_hdr;
	}

	c->setup.u.us.sock_name = NULL;
	c->request.u.us.sock_name = NULL;

	/* response channel */
	c->response.u.us.sock = c->request.u.us.sock;
	snprintf(path, PATH_MAX, "%s-%s", r->response, "response");
	c->response.u.us.sock_name = strdup(path);

	/* event channel */
	res = qb_ipc_dgram_sock_setup(r->response, "event-tx",
				      &c->event.u.us.sock, c->egid);
	if (res < 0) {
		goto cleanup_hdr;
	}

	res = set_sock_size(c->event.u.us.sock, c->event.max_msg_size);
	if (res != 0) {
		goto cleanup_hdr;
	}

	snprintf(path, PATH_MAX, "%s-%s", r->response, "event");
	c->event.u.us.sock_name = strdup(path);

	res = _sock_add_to_mainloop(c);
	if (res < 0) {
		goto cleanup_hdr;
	}

	return res;

cleanup_hdr:
	free(c->response.u.us.sock_name);
	free(c->event.u.us.sock_name);

	if (fd_hdr >= 0) {
		close(fd_hdr);
	}
	unlink(r->request);
	munmap(c->request.u.us.shared_data, SHM_CONTROL_SIZE);
	return res;
}