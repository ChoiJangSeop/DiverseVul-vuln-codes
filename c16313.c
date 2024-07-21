int dtls1_read_failed(SSL *s, int code)
	{
	if ( code > 0)
		{
		fprintf( stderr, "invalid state reached %s:%d", __FILE__, __LINE__);
		return 1;
		}

	if (!dtls1_is_timer_expired(s))
		{
		/* not a timeout, none of our business, 
		   let higher layers handle this.  in fact it's probably an error */
		return code;
		}

	if ( ! SSL_in_init(s))  /* done, no need to send a retransmit */
		{
		BIO_set_flags(SSL_get_rbio(s), BIO_FLAGS_READ);
		return code;
		}

#if 0 /* for now, each alert contains only one record number */
	item = pqueue_peek(state->rcvd_records);
	if ( item )
		{
		/* send an alert immediately for all the missing records */
		}
	else
#endif

#if 0  /* no more alert sending, just retransmit the last set of messages */
	if ( state->timeout.read_timeouts >= DTLS1_TMO_READ_COUNT)
		ssl3_send_alert(s,SSL3_AL_WARNING,
			DTLS1_AD_MISSING_HANDSHAKE_MESSAGE);
#endif

	return dtls1_handle_timeout(s);
	}