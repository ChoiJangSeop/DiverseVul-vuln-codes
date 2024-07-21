int ssl_check_tlsext(SSL *s, int is_server)
	{
	int ret=SSL_TLSEXT_ERR_NOACK;

	int al = SSL_AD_UNRECOGNIZED_NAME;

	if (s->ctx != NULL && s->ctx->tlsext_servername_callback != 0) 
		ret = s->ctx->tlsext_servername_callback(s, &al, s->ctx->tlsext_servername_arg);
	else if (s->initial_ctx != NULL && s->initial_ctx->tlsext_servername_callback != 0) 		
		ret = s->initial_ctx->tlsext_servername_callback(s, &al, s->initial_ctx->tlsext_servername_arg);

	switch (ret) {
		case SSL_TLSEXT_ERR_ALERT_FATAL:
			ssl3_send_alert(s,SSL3_AL_FATAL,al); 
			return -1;

		case SSL_TLSEXT_ERR_ALERT_WARNING:
			ssl3_send_alert(s,SSL3_AL_WARNING,al);
			return 1; 
					
		case SSL_TLSEXT_ERR_NOACK:
			s->servername_done=0;
			default:
		return 1;
	}
}