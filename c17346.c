static int post_msg(struct pptp_conn_t *conn, void *buf, int size)
{
	int n;
	if (conn->out_size) {
		log_error("pptp: buffer is not empty\n");
		return -1;
	}

again:
	n=write(conn->hnd.fd, buf, size);
	if (n < 0) {
		if (errno == EINTR)
			goto again;
		else if (errno == EAGAIN)
			n = 0;
		else {
			if (errno != EPIPE) {
				if (conf_verbose)
					log_ppp_info2("pptp: write: %s\n", strerror(errno));
				return -1;
			}
		}
	}

	if ( n<size ) {
		memcpy(conn->out_buf, (uint8_t *)buf + n, size - n);
		triton_md_enable_handler(&conn->hnd, MD_MODE_WRITE);
	}

	return 0;
}