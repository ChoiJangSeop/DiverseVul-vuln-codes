ftp_login(ftpbuf_t *ftp, const char *user, const char *pass TSRMLS_DC)
{
#if HAVE_OPENSSL_EXT
	SSL_CTX	*ctx = NULL;
	long ssl_ctx_options = SSL_OP_ALL;
#endif
	if (ftp == NULL) {
		return 0;
	}

#if HAVE_OPENSSL_EXT
	if (ftp->use_ssl && !ftp->ssl_active) {
		if (!ftp_putcmd(ftp, "AUTH", "TLS")) {
			return 0;
		}
		if (!ftp_getresp(ftp)) {
			return 0;
		}
			
		if (ftp->resp != 234) {
			if (!ftp_putcmd(ftp, "AUTH", "SSL")) {
				return 0;
			}
			if (!ftp_getresp(ftp)) {
				return 0;
			}
				
			if (ftp->resp != 334) {
				return 0;
			} else {
				ftp->old_ssl = 1;
				ftp->use_ssl_for_data = 1;
			}
		}
		
		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "failed to create the SSL context");
			return 0;
		}

#if OPENSSL_VERSION_NUMBER >= 0x0090605fL
		ssl_ctx_options &= ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
#endif
		SSL_CTX_set_options(ctx, ssl_ctx_options);

		ftp->ssl_handle = SSL_new(ctx);
		if (ftp->ssl_handle == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "failed to create the SSL handle");
			SSL_CTX_free(ctx);
			return 0;
		}

		SSL_set_fd(ftp->ssl_handle, ftp->fd);

		if (SSL_connect(ftp->ssl_handle) <= 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "SSL/TLS handshake failed");
			SSL_shutdown(ftp->ssl_handle);
			SSL_free(ftp->ssl_handle);
			return 0;
		}

		ftp->ssl_active = 1;

		if (!ftp->old_ssl) {

			/* set protection buffersize to zero */
			if (!ftp_putcmd(ftp, "PBSZ", "0")) {
				return 0;
			}
			if (!ftp_getresp(ftp)) {
				return 0;
			}

			/* enable data conn encryption */
			if (!ftp_putcmd(ftp, "PROT", "P")) {
				return 0;
			}
			if (!ftp_getresp(ftp)) {
				return 0;
			}
			
			ftp->use_ssl_for_data = (ftp->resp >= 200 && ftp->resp <=299);		
		}
	}
#endif

	if (!ftp_putcmd(ftp, "USER", user)) {
		return 0;
	}
	if (!ftp_getresp(ftp)) {
		return 0;
	}
	if (ftp->resp == 230) {
		return 1;
	}
	if (ftp->resp != 331) {
		return 0;
	}
	if (!ftp_putcmd(ftp, "PASS", pass)) {
		return 0;
	}
	if (!ftp_getresp(ftp)) {
		return 0;
	}
	return (ftp->resp == 230);
}