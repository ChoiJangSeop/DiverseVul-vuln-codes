R_API int r_socket_block_time(RSocket *s, int block, int sec, int usec) {
#if __UNIX__
	int ret, flags;
#endif
	if (!s) {
		return false;
	}
#if __UNIX__
	flags = fcntl (s->fd, F_GETFL, 0);
	if (flags < 0) {
		return false;
	}
	ret = fcntl (s->fd, F_SETFL, block?
			(flags & ~O_NONBLOCK):
			(flags | O_NONBLOCK));
	if (ret < 0) {
		return false;
	}
#elif __WINDOWS__
	ioctlsocket (s->fd, FIONBIO, (u_long FAR*)&block);
#endif
	if (sec > 0 || usec > 0) {
		struct timeval tv = {0};
		tv.tv_sec = sec;
		tv.tv_usec = usec;
		if (setsockopt (s->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof (tv)) < 0) {
			return false;
		}
	}
	return true;
}