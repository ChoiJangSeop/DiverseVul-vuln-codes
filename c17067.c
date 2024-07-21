static void server(int fd, unsigned rounds, const char *prio)
{
	int retval;
	gnutls_session_t session;
	gnutls_datum_t session_ticket_key = { NULL, 0 };
	gnutls_certificate_credentials_t serverx509cred = NULL;

	virt_time_init();

	if (gnutls_session_ticket_key_generate(&session_ticket_key) < 0) {
		fail("server: Could not generate session ticket key\n");
	}

	for (unsigned i = 0; i < rounds; i++) {
		assert(gnutls_init(&session, GNUTLS_SERVER)>=0);

		assert(gnutls_certificate_allocate_credentials(&serverx509cred)>=0);
		retval = gnutls_certificate_set_x509_key_mem(serverx509cred,
						&server_cert, &server_key,
						GNUTLS_X509_FMT_PEM);
		if (retval < 0)
			fail("error setting key: %s\n", gnutls_strerror(retval));

		assert(gnutls_priority_set_direct(session, prio, NULL)>=0);
		gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, serverx509cred);

		gnutls_db_set_cache_expiration(session, TICKET_EXPIRATION);
		_gnutls_set_session_ticket_key_rotation_callback(session, stek_rotation_callback);

		retval = gnutls_session_ticket_enable_server(session,
							     &session_ticket_key);
		if (retval != GNUTLS_E_SUCCESS) {
			fail("server: Could not enable session tickets: %s\n", gnutls_strerror(retval));
		}

		gnutls_transport_set_int(session, fd);
		gnutls_handshake_set_timeout(session, 20 * 1000);

		virt_sec_sleep(TICKET_ROTATION_PERIOD-1);

		do {
			retval = gnutls_handshake(session);
		} while (retval == GNUTLS_E_AGAIN || retval == GNUTLS_E_INTERRUPTED);

		if (retval < 0) {
			fail("server: Handshake failed: %s\n", gnutls_strerror(retval));
		} else {
			success("server: Handshake was completed\n");
		}

		if (gnutls_session_is_resumed(session))
			fail("server: Session was resumed (but should not)\n");
		else
			success("server: Success: Session was NOT resumed\n");

		gnutls_bye(session, GNUTLS_SHUT_RDWR);
		gnutls_deinit(session);
		gnutls_certificate_free_credentials(serverx509cred);
		serverx509cred = NULL;
	}

	if (num_stek_rotations != 2)
		fail("STEK should be rotated exactly twice (%d)!\n", num_stek_rotations);

	if (serverx509cred)
		gnutls_certificate_free_credentials(serverx509cred);
	gnutls_free(session_ticket_key.data);
}