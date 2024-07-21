R_API int r_socket_ready(RSocket *s, int secs, int usecs) {
#if __UNIX__
	//int msecs = (1000 * secs) + (usecs / 1000);
	int msecs = (usecs / 1000);
	struct pollfd fds[1];
	fds[0].fd = s->fd;
	fds[0].events = POLLIN | POLLPRI;
	fds[0].revents = POLLNVAL | POLLHUP | POLLERR;
	return poll ((struct pollfd *)&fds, 1, msecs);
#elif __WINDOWS__
	fd_set rfds;
	struct timeval tv;
	if (s->fd == R_INVALID_SOCKET) {
		return -1;
	}
	FD_ZERO (&rfds);
	FD_SET (s->fd, &rfds);
	tv.tv_sec = secs;
	tv.tv_usec = usecs;
	return select (s->fd + 1, &rfds, NULL, NULL, &tv);
#else
	return true; /* always ready if unknown */
#endif
}