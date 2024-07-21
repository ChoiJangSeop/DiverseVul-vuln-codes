R_API RSocket *r_socket_accept_timeout(RSocket *s, unsigned int timeout) {
	fd_set read_fds;
	fd_set except_fds;

	FD_ZERO (&read_fds);
	FD_SET (s->fd, &read_fds);

	FD_ZERO (&except_fds);
	FD_SET (s->fd, &except_fds);

	struct timeval t;
	t.tv_sec = timeout;
	t.tv_usec = 0;

	int r = select (s->fd + 1, &read_fds, NULL, &except_fds, &t);
	if(r < 0) {
		perror ("select");
	} else if (r > 0 && FD_ISSET (s->fd, &read_fds)) {
		return r_socket_accept (s);
	}

	return NULL;
}