int gnutls_init(gnutls_session_t * session, unsigned int flags)
{
	int ret;

	FAIL_IF_LIB_ERROR;

	*session = gnutls_calloc(1, sizeof(struct gnutls_session_int));
	if (*session == NULL)
		return GNUTLS_E_MEMORY_ERROR;

	ret = gnutls_mutex_init(&(*session)->internals.post_negotiation_lock);
	if (ret < 0) {
		gnutls_assert();
		gnutls_free(*session);
		return ret;
	}

	ret = gnutls_mutex_init(&(*session)->internals.epoch_lock);
	if (ret < 0) {
		gnutls_assert();
		gnutls_mutex_deinit(&(*session)->internals.post_negotiation_lock);
		gnutls_free(*session);
		return ret;
	}

	ret = _gnutls_epoch_setup_next(*session, 1, NULL);
	if (ret < 0) {
		gnutls_mutex_deinit(&(*session)->internals.post_negotiation_lock);
		gnutls_mutex_deinit(&(*session)->internals.epoch_lock);
		gnutls_free(*session);
		return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
	}
	_gnutls_epoch_bump(*session);

	(*session)->security_parameters.entity =
	    (flags & GNUTLS_SERVER ? GNUTLS_SERVER : GNUTLS_CLIENT);

	/* the default certificate type for TLS */
	(*session)->security_parameters.client_ctype = DEFAULT_CERT_TYPE;
	(*session)->security_parameters.server_ctype = DEFAULT_CERT_TYPE;

	/* Initialize buffers */
	_gnutls_buffer_init(&(*session)->internals.handshake_hash_buffer);
	_gnutls_buffer_init(&(*session)->internals.post_handshake_hash_buffer);
	_gnutls_buffer_init(&(*session)->internals.hb_remote_data);
	_gnutls_buffer_init(&(*session)->internals.hb_local_data);
	_gnutls_buffer_init(&(*session)->internals.record_presend_buffer);
	_gnutls_buffer_init(&(*session)->internals.record_key_update_buffer);
	_gnutls_buffer_init(&(*session)->internals.reauth_buffer);

	_mbuffer_head_init(&(*session)->internals.record_buffer);
	_mbuffer_head_init(&(*session)->internals.record_send_buffer);
	_mbuffer_head_init(&(*session)->internals.record_recv_buffer);
	_mbuffer_head_init(&(*session)->internals.early_data_recv_buffer);
	_gnutls_buffer_init(&(*session)->internals.early_data_presend_buffer);

	_mbuffer_head_init(&(*session)->internals.handshake_send_buffer);
	_gnutls_handshake_recv_buffer_init(*session);

	(*session)->internals.expire_time = DEFAULT_EXPIRE_TIME;

	/* Ticket key rotation - set the default X to 3 times the ticket expire time */
	(*session)->key.totp.last_result = 0;

	gnutls_handshake_set_max_packet_length((*session),
					       MAX_HANDSHAKE_PACKET_SIZE);

	/* set the socket pointers to -1;
	 */
	(*session)->internals.transport_recv_ptr =
	    (gnutls_transport_ptr_t) - 1;
	(*session)->internals.transport_send_ptr =
	    (gnutls_transport_ptr_t) - 1;

	/* set the default maximum record size for TLS
	 */
	(*session)->security_parameters.max_record_recv_size =
	    DEFAULT_MAX_RECORD_SIZE;
	(*session)->security_parameters.max_record_send_size =
	    DEFAULT_MAX_RECORD_SIZE;
	(*session)->security_parameters.max_user_record_recv_size =
	    DEFAULT_MAX_RECORD_SIZE;
	(*session)->security_parameters.max_user_record_send_size =
	    DEFAULT_MAX_RECORD_SIZE;

	/* set the default early data size for TLS
	 */
	if ((*session)->security_parameters.entity == GNUTLS_SERVER) {
		(*session)->security_parameters.max_early_data_size =
			DEFAULT_MAX_EARLY_DATA_SIZE;
	} else {
		(*session)->security_parameters.max_early_data_size =
			UINT32_MAX;
	}

	/* Everything else not initialized here is initialized as NULL
	 * or 0. This is why calloc is used. However, we want to
	 * ensure that certain portions of data are initialized at
	 * runtime before being used. Mark such regions with a
	 * valgrind client request as undefined.
	 */
#ifdef HAVE_VALGRIND_MEMCHECK_H
	if (RUNNING_ON_VALGRIND) {
		if (flags & GNUTLS_CLIENT)
			VALGRIND_MAKE_MEM_UNDEFINED((*session)->security_parameters.client_random,
						    GNUTLS_RANDOM_SIZE);
		if (flags & GNUTLS_SERVER)
			VALGRIND_MAKE_MEM_UNDEFINED((*session)->security_parameters.server_random,
						    GNUTLS_RANDOM_SIZE);
	}
#endif
	handshake_internal_state_clear1(*session);

#ifdef HAVE_WRITEV
#ifdef MSG_NOSIGNAL
	if (flags & GNUTLS_NO_SIGNAL)
		gnutls_transport_set_vec_push_function(*session, system_writev_nosignal);
	else
#endif
		gnutls_transport_set_vec_push_function(*session, system_writev);
#else
	gnutls_transport_set_push_function(*session, system_write);
#endif
	(*session)->internals.pull_timeout_func = gnutls_system_recv_timeout;
	(*session)->internals.pull_func = system_read;
	(*session)->internals.errno_func = system_errno;

	(*session)->internals.saved_username_size = -1;

	/* heartbeat timeouts */
	(*session)->internals.hb_retrans_timeout_ms = 1000;
	(*session)->internals.hb_total_timeout_ms = 60000;

	if (flags & GNUTLS_DATAGRAM) {
		(*session)->internals.dtls.mtu = DTLS_DEFAULT_MTU;
		(*session)->internals.transport = GNUTLS_DGRAM;

		gnutls_dtls_set_timeouts(*session, DTLS_RETRANS_TIMEOUT, 60000);
	} else {
		(*session)->internals.transport = GNUTLS_STREAM;
	}

	/* Enable useful extensions */
	if ((flags & GNUTLS_CLIENT) && !(flags & GNUTLS_NO_EXTENSIONS)) {
#ifdef ENABLE_OCSP
		gnutls_ocsp_status_request_enable_client(*session, NULL, 0,
							 NULL);
#endif
	}

	/* session tickets in server side are enabled by setting a key */
	if (flags & GNUTLS_SERVER)
		flags |= GNUTLS_NO_TICKETS;

	(*session)->internals.flags = flags;

	if (_gnutls_disable_tls13 != 0)
		(*session)->internals.flags |= INT_FLAG_NO_TLS13;

	/* Install the default keylog function */
	gnutls_session_set_keylog_function(*session, _gnutls_nss_keylog_func);

	return 0;
}