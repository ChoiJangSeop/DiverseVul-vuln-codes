handle_new_connection(struct qb_ipcs_service *s,
		      int32_t auth_result,
		      int32_t sock,
		      void *msg, size_t len, struct ipc_auth_ugp *ugp)
{
	struct qb_ipcs_connection *c = NULL;
	struct qb_ipc_connection_request *req = msg;
	int32_t res = auth_result;
	int32_t res2 = 0;
	uint32_t max_buffer_size = QB_MAX(req->max_msg_size, s->max_buffer_size);
	struct qb_ipc_connection_response response;

	c = qb_ipcs_connection_alloc(s);
	if (c == NULL) {
		qb_ipcc_us_sock_close(sock);
		return -ENOMEM;
	}

	c->receive_buf = calloc(1, max_buffer_size);
	if (c->receive_buf == NULL) {
		free(c);
		qb_ipcc_us_sock_close(sock);
		return -ENOMEM;
	}
	c->setup.u.us.sock = sock;
	c->request.max_msg_size = max_buffer_size;
	c->response.max_msg_size = max_buffer_size;
	c->event.max_msg_size = max_buffer_size;
	c->pid = ugp->pid;
	c->auth.uid = c->euid = ugp->uid;
	c->auth.gid = c->egid = ugp->gid;
	c->auth.mode = 0600;
	c->stats.client_pid = ugp->pid;
	snprintf(c->description, CONNECTION_DESCRIPTION,
		 "%d-%d-%d", s->pid, ugp->pid, c->setup.u.us.sock);

	if (auth_result == 0 && c->service->serv_fns.connection_accept) {
		res = c->service->serv_fns.connection_accept(c,
							     c->euid, c->egid);
	}
	if (res != 0) {
		goto send_response;
	}

	qb_util_log(LOG_DEBUG, "IPC credentials authenticated (%s)",
		    c->description);

	memset(&response, 0, sizeof(response));
	if (s->funcs.connect) {
		res = s->funcs.connect(s, c, &response);
		if (res != 0) {
			goto send_response;
		}
	}
	/*
	 * The connection is good, add it to the active connection list
	 */
	c->state = QB_IPCS_CONNECTION_ACTIVE;
	qb_list_add(&c->list, &s->connections);

send_response:
	response.hdr.id = QB_IPC_MSG_AUTHENTICATE;
	response.hdr.size = sizeof(response);
	response.hdr.error = res;
	if (res == 0) {
		response.connection = (intptr_t) c;
		response.connection_type = s->type;
		response.max_msg_size = c->request.max_msg_size;
		s->stats.active_connections++;
	}

	res2 = qb_ipc_us_send(&c->setup, &response, response.hdr.size);
	if (res == 0 && res2 != response.hdr.size) {
		res = res2;
	}

	if (res == 0) {
		qb_ipcs_connection_ref(c);
		if (s->serv_fns.connection_created) {
			s->serv_fns.connection_created(c);
		}
		if (c->state == QB_IPCS_CONNECTION_ACTIVE) {
			c->state = QB_IPCS_CONNECTION_ESTABLISHED;
		}
		qb_ipcs_connection_unref(c);
	} else {
		if (res == -EACCES) {
			qb_util_log(LOG_ERR, "Invalid IPC credentials (%s).",
				    c->description);
		} else if (res == -EAGAIN) {
			qb_util_log(LOG_WARNING, "Denied connection, is not ready (%s)",
				    c->description);
		} else {
			errno = -res;
			qb_util_perror(LOG_ERR,
				       "Error in connection setup (%s)",
				       c->description);
		}

		if (c->state == QB_IPCS_CONNECTION_INACTIVE) {
			/* This removes the initial alloc ref */
			qb_ipcs_connection_unref(c);
		        qb_ipcc_us_sock_close(sock);
		} else {
			qb_ipcs_disconnect(c);
		}
	}
	return res;
}