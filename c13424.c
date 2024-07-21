listen_loop(void *x) {
    int			optval = 1;
    struct pollfd	pa[100];
    struct pollfd	*p;
    struct _agooErr	err = AGOO_ERR_INIT;
    struct sockaddr_in	client_addr;
    int			client_sock;
    int			pcnt = 0;
    socklen_t		alen = 0;
    agooCon		con;
    int			i;
    uint64_t		cnt = 0;
    agooBind		b;

    for (b = agoo_server.binds, p = pa; NULL != b; b = b->next, p++, pcnt++) {
	p->fd = b->fd;
	p->events = POLLIN;
	p->revents = 0;
    }
    memset(&client_addr, 0, sizeof(client_addr));
    atomic_fetch_add(&agoo_server.running, 1);
    while (agoo_server.active) {
	if (0 > (i = poll(pa, pcnt, 200))) {
	    if (EAGAIN == errno) {
		continue;
	    }
	    agoo_log_cat(&agoo_error_cat, "Server polling error. %s.", strerror(errno));
	    // Either a signal or something bad like out of memory. Might as well exit.
	    break;
	}
	if (0 == i) { // nothing to read
	    continue;
	}
	for (b = agoo_server.binds, p = pa; NULL != b; b = b->next, p++) {
	    if (0 != (p->revents & POLLIN)) {
		if (0 > (client_sock = accept(p->fd, (struct sockaddr*)&client_addr, &alen))) {
		    agoo_log_cat(&agoo_error_cat, "Server with pid %d accept connection failed. %s.", getpid(), strerror(errno));
		} else if (NULL == (con = agoo_con_create(&err, client_sock, ++cnt, b))) {
		    agoo_log_cat(&agoo_error_cat, "Server with pid %d accept connection failed. %s.", getpid(), err.msg);
		    close(client_sock);
		    cnt--;
		    agoo_err_clear(&err);
		} else {
		    int	con_cnt;
#ifdef OSX_OS
		    setsockopt(client_sock, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval));
#endif
#ifdef PLATFORM_LINUX
		    setsockopt(client_sock, IPPROTO_TCP, TCP_QUICKACK, &optval, sizeof(optval));
#endif
		    fcntl(client_sock, F_SETFL, O_NONBLOCK);
		    //fcntl(client_sock, F_SETFL, FNDELAY);
		    setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
		    setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
		    agoo_log_cat(&agoo_con_cat, "Server with pid %d accepted connection %llu on %s [%d]",
				 getpid(), (unsigned long long)cnt, b->id, con->sock);

		    con_cnt = atomic_fetch_add(&agoo_server.con_cnt, 1);
		    if (agoo_server.loop_max > agoo_server.loop_cnt && agoo_server.loop_cnt * LOOP_UP < con_cnt) {
			add_con_loop();
		    }
		    agoo_queue_push(&agoo_server.con_queue, (void*)con);
		}
	    }
	    if (0 != (p->revents & (POLLERR | POLLHUP | POLLNVAL))) {
		if (0 != (p->revents & (POLLHUP | POLLNVAL))) {
		    agoo_log_cat(&agoo_error_cat, "Agoo server with pid %d socket on %s closed.", getpid(), b->id);
		} else {
		    agoo_log_cat(&agoo_error_cat, "Agoo server with pid %d socket on %s error.", getpid(), b->id);
		}
		agoo_server.active = false;
	    }
	    p->revents = 0;
	}
    }
    for (b = agoo_server.binds; NULL != b; b = b->next) {
	agoo_bind_close(b);
    }
    atomic_fetch_sub(&agoo_server.running, 1);

    return NULL;
}