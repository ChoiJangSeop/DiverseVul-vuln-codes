int dtls1_handle_timeout(SSL *s)
	{
	DTLS1_STATE *state;

	/* if no timer is expired, don't do anything */
	if (!dtls1_is_timer_expired(s))
		{
		return 0;
		}

	dtls1_double_timeout(s);
	state = s->d1;
	state->timeout.num_alerts++;
	if ( state->timeout.num_alerts > DTLS1_TMO_ALERT_COUNT)
		{
		/* fail the connection, enough alerts have been sent */
		SSLerr(SSL_F_DTLS1_HANDLE_TIMEOUT,SSL_R_READ_TIMEOUT_EXPIRED);
		return -1;
		}

	state->timeout.read_timeouts++;
	if ( state->timeout.read_timeouts > DTLS1_TMO_READ_COUNT)
		{
		state->timeout.read_timeouts = 1;
		}

	dtls1_start_timer(s);
	return dtls1_retransmit_buffered_messages(s);
	}