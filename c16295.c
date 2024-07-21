static int init_ssl_connection(SSL *con)
	{
	int i;
	const char *str;
	X509 *peer;
	long verify_error;
	MS_STATIC char buf[BUFSIZ];

	if ((i=SSL_accept(con)) <= 0)
		{
		if (BIO_sock_should_retry(i))
			{
			BIO_printf(bio_s_out,"DELAY\n");
			return(1);
			}

		BIO_printf(bio_err,"ERROR\n");
		verify_error=SSL_get_verify_result(con);
		if (verify_error != X509_V_OK)
			{
			BIO_printf(bio_err,"verify error:%s\n",
				X509_verify_cert_error_string(verify_error));
			}
		else
			ERR_print_errors(bio_err);
		return(0);
		}

	PEM_write_bio_SSL_SESSION(bio_s_out,SSL_get_session(con));

	peer=SSL_get_peer_certificate(con);
	if (peer != NULL)
		{
		BIO_printf(bio_s_out,"Client certificate\n");
		PEM_write_bio_X509(bio_s_out,peer);
		X509_NAME_oneline(X509_get_subject_name(peer),buf,sizeof buf);
		BIO_printf(bio_s_out,"subject=%s\n",buf);
		X509_NAME_oneline(X509_get_issuer_name(peer),buf,sizeof buf);
		BIO_printf(bio_s_out,"issuer=%s\n",buf);
		X509_free(peer);
		}

	if (SSL_get_shared_ciphers(con,buf,sizeof buf) != NULL)
		BIO_printf(bio_s_out,"Shared ciphers:%s\n",buf);
	str=SSL_CIPHER_get_name(SSL_get_current_cipher(con));
	BIO_printf(bio_s_out,"CIPHER is %s\n",(str != NULL)?str:"(NONE)");
	if (con->hit) BIO_printf(bio_s_out,"Reused session-id\n");
	if (SSL_ctrl(con,SSL_CTRL_GET_FLAGS,0,NULL) &
		TLS1_FLAGS_TLS_PADDING_BUG)
		BIO_printf(bio_s_out,"Peer has incorrect TLSv1 block padding\n");
#ifndef OPENSSL_NO_KRB5
	if (con->kssl_ctx->client_princ != NULL)
		{
		BIO_printf(bio_s_out,"Kerberos peer principal is %s\n",
			con->kssl_ctx->client_princ);
		}
#endif /* OPENSSL_NO_KRB5 */
	BIO_printf(bio_s_out, "Secure Renegotiation IS%s supported\n",
		      SSL_get_secure_renegotiation_support(con) ? "" : " NOT");
	return(1);
	}