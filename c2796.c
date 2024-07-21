my_send(ftpbuf_t *ftp, php_socket_t s, void *buf, size_t len)
{
	long		size, sent;
    int         n;

	size = len;
	while (size) {
		n = php_pollfd_for_ms(s, POLLOUT, ftp->timeout_sec * 1000);

		if (n < 1) {

#if !defined(PHP_WIN32) && !(defined(NETWARE) && defined(USE_WINSOCK))
			if (n == 0) {
				errno = ETIMEDOUT;
			}
#endif
			return -1;
		}

#if HAVE_OPENSSL_EXT
		if (ftp->use_ssl && ftp->fd == s && ftp->ssl_active) {
			sent = SSL_write(ftp->ssl_handle, buf, size);
		} else if (ftp->use_ssl && ftp->fd != s && ftp->use_ssl_for_data && ftp->data->ssl_active) {	
			sent = SSL_write(ftp->data->ssl_handle, buf, size);
		} else {
#endif
			sent = send(s, buf, size, 0);
#if HAVE_OPENSSL_EXT
		}
#endif
		if (sent == -1) {
			return -1;
		}

		buf = (char*) buf + sent;
		size -= sent;
	}

	return len;
}