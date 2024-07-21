R_API bool r_socket_connect(RSocket *s, const char *host, const char *port, int proto, unsigned int timeout) {
	r_return_val_if_fail (s, false);
#if __WINDOWS__
	struct sockaddr_in sa;
	struct hostent *he;
	WSADATA wsadata;
	TIMEVAL Timeout;
	Timeout.tv_sec = timeout;
	Timeout.tv_usec = 0;

	if (WSAStartup (MAKEWORD (1, 1), &wsadata) == SOCKET_ERROR) {
		eprintf ("Error creating socket.");
		return false;
	}
	s->fd = socket (AF_INET, SOCK_STREAM, 0);
	if (s->fd == R_INVALID_SOCKET) {
		return false;
	}

	unsigned long iMode = 1;
	int iResult = ioctlsocket (s->fd, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		eprintf ("ioctlsocket error: %d\n", iResult);
	}
	memset (&sa, 0, sizeof (sa));
	sa.sin_family = AF_INET;
	he = (struct hostent *)gethostbyname (host);
	if (he == (struct hostent*)0) {
#ifdef _MSC_VER
		closesocket (s->fd);
#else
		close (s->fd);
#endif
		return false;
	}
	sa.sin_addr = *((struct in_addr *)he->h_addr);
	s->port = r_socket_port_by_name (port);
	s->proto = proto;
	sa.sin_port = htons (s->port);
	if (!connect (s->fd, (const struct sockaddr*)&sa, sizeof (struct sockaddr))) {
#ifdef _MSC_VER
		closesocket (s->fd);
#else
		close (s->fd);
#endif
		return false;
	}
	iMode = 0;
	iResult = ioctlsocket (s->fd, FIONBIO, &iMode);
	if (iResult != NO_ERROR) {
		eprintf ("ioctlsocket error: %d\n", iResult);
	}
	if (timeout > 0) {
		r_socket_block_time (s, 1, timeout, 0);
	}
	fd_set Write, Err;
	FD_ZERO (&Write);
	FD_ZERO (&Err);
	FD_SET (s->fd, &Write);
	FD_SET (s->fd, &Err);
	select (0, NULL, &Write, &Err, &Timeout);
	if (FD_ISSET (s->fd, &Write)) {
		return true;
	}
	return false;
#elif __UNIX__
	int ret;
	struct addrinfo hints = {0};
	struct addrinfo *res, *rp;
	if (!proto) {
		proto = R_SOCKET_PROTO_TCP;
	}
	r_sys_signal (SIGPIPE, SIG_IGN);
	if (proto == R_SOCKET_PROTO_UNIX) {
		if (!__connect_unix (s, host)) {
			return false;
		}
	} else {
		hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
		hints.ai_protocol = proto;
		int gai = getaddrinfo (host, port, &hints, &res);
		if (gai != 0) {
			eprintf ("r_socket_connect: Error in getaddrinfo: %s (%s:%s)\n",
				gai_strerror (gai), host, port);
			return false;
		}
		for (rp = res; rp != NULL; rp = rp->ai_next) {
			int flag = 1;

			s->fd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			if (s->fd == -1) {
				perror ("socket");
				continue;
			}
			ret = setsockopt (s->fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof (flag));
			if (ret < 0) {
				perror ("setsockopt");
				close (s->fd);
				s->fd = -1;
				continue;
			}

			r_socket_block_time (s, 0, 0, 0);
			ret = connect (s->fd, rp->ai_addr, rp->ai_addrlen);

			if (ret == 0) {
				freeaddrinfo (res);
				return true;
			}
			if (errno == EINPROGRESS) {
				struct timeval tv;
				tv.tv_sec = timeout;
				tv.tv_usec = 0;
				fd_set wfds;
				FD_ZERO(&wfds);
				FD_SET(s->fd, &wfds);

				if ((ret = select (s->fd + 1, NULL, &wfds, NULL, &tv)) != -1) {
					if (r_socket_is_connected (s)) {
						freeaddrinfo (res);
						return true;
					}
				} else {
					perror ("connect");
				}
			}
			r_socket_close (s);
		}
		freeaddrinfo (res);
		if (!rp) {
			eprintf ("Could not resolve address '%s' or failed to connect\n", host);
			return false;
		}
	}
#endif
#if HAVE_LIB_SSL
	if (s->is_ssl) {
		s->ctx = SSL_CTX_new (SSLv23_client_method ());
		if (!s->ctx) {
			r_socket_free (s);
			return false;
		}
		s->sfd = SSL_new (s->ctx);
		SSL_set_fd (s->sfd, s->fd);
		if (SSL_connect (s->sfd) != 1) {
			r_socket_free (s);
			return false;
		}
	}
#endif
	return true;
}