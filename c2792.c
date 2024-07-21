ftp_close(ftpbuf_t *ftp)
{
	if (ftp == NULL) {
		return NULL;
	}
	if (ftp->data) {
		data_close(ftp, ftp->data);
	}
	if (ftp->stream && ftp->closestream) {
		TSRMLS_FETCH();
		php_stream_close(ftp->stream);
	}
	if (ftp->fd != -1) {
#if HAVE_OPENSSL_EXT
		if (ftp->ssl_active) {
			SSL_shutdown(ftp->ssl_handle);
			SSL_free(ftp->ssl_handle);
		}
#endif		
		closesocket(ftp->fd);
	}	
	ftp_gc(ftp);
	efree(ftp);
	return NULL;
}