my_recv(ftpbuf_t *ftp, php_socket_t s, void *buf, size_t len)
{
	int		n, nr_bytes;

	n = php_pollfd_for_ms(s, PHP_POLLREADABLE, ftp->timeout_sec * 1000);
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
		nr_bytes = SSL_read(ftp->ssl_handle, buf, len);
	} else if (ftp->use_ssl && ftp->fd != s && ftp->use_ssl_for_data && ftp->data->ssl_active) {	
		nr_bytes = SSL_read(ftp->data->ssl_handle, buf, len);
	} else {
#endif
		nr_bytes = recv(s, buf, len, 0);
#if HAVE_OPENSSL_EXT
	}
#endif	
	return (nr_bytes);
}