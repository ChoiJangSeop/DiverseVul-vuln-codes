data_accept(databuf_t *data, ftpbuf_t *ftp TSRMLS_DC)
{
	php_sockaddr_storage addr;
	socklen_t			size;

#if HAVE_OPENSSL_EXT
	SSL_CTX		*ctx;
	long ssl_ctx_options = SSL_OP_ALL;
#endif

	if (data->fd != -1) {
		goto data_accepted;
	}
	size = sizeof(addr);
	data->fd = my_accept(ftp, data->listener, (struct sockaddr*) &addr, &size);
	closesocket(data->listener);
	data->listener = -1;

	if (data->fd == -1) {
		efree(data);
		return NULL;
	}

data_accepted:
#if HAVE_OPENSSL_EXT
	
	/* now enable ssl if we need to */
	if (ftp->use_ssl && ftp->use_ssl_for_data) {
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "data_accept: failed to create the SSL context");
			return 0;
		}

#if OPENSSL_VERSION_NUMBER >= 0x0090605fL
		ssl_ctx_options &= ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
#endif
		SSL_CTX_set_options(ctx, ssl_ctx_options);

		data->ssl_handle = SSL_new(ctx);
		if (data->ssl_handle == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "data_accept: failed to create the SSL handle");
			SSL_CTX_free(ctx);
			return 0;
		}
			
		
		SSL_set_fd(data->ssl_handle, data->fd);

		if (ftp->old_ssl) {
			SSL_copy_session_id(data->ssl_handle, ftp->ssl_handle);
		}
			
		if (SSL_connect(data->ssl_handle) <= 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "data_accept: SSL/TLS handshake failed");
			SSL_shutdown(data->ssl_handle);
			SSL_free(data->ssl_handle);
			return 0;
		}
			
		data->ssl_active = 1;
	}	

#endif

	return data;
}