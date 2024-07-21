data_close(ftpbuf_t *ftp, databuf_t *data)
{
#if HAVE_OPENSSL_EXT
	SSL_CTX		*ctx;
#endif				
	if (data == NULL) {
		return NULL;
	}
	if (data->listener != -1) {
#if HAVE_OPENSSL_EXT
		if (data->ssl_active) {
		
			ctx = SSL_get_SSL_CTX(data->ssl_handle);
			SSL_CTX_free(ctx);

			SSL_shutdown(data->ssl_handle);
			SSL_free(data->ssl_handle);
			data->ssl_active = 0;
		}
#endif				
		closesocket(data->listener);
	}	
	if (data->fd != -1) {
#if HAVE_OPENSSL_EXT
		if (data->ssl_active) {
			ctx = SSL_get_SSL_CTX(data->ssl_handle);
			SSL_CTX_free(ctx);

			SSL_shutdown(data->ssl_handle);
			SSL_free(data->ssl_handle);
			data->ssl_active = 0;
		}
#endif				
		closesocket(data->fd);
	}	
	if (ftp) {
		ftp->data = NULL;
	}
	efree(data);
	return NULL;
}