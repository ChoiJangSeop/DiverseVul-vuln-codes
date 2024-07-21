static int rotate(gnutls_session_t session)
{
	int64_t t;
	gnutls_datum_t secret;
	uint8_t key[TICKET_MASTER_KEY_SIZE];

	/* Do we need to calculate new totp? */
	t = totp_next(session);
	if (t > 0) {
		secret.data = session->key.initial_stek;
		secret.size = TICKET_MASTER_KEY_SIZE;

		/* Generate next key */
		if (totp_sha3(session, t, &secret, key) < 0) {
			gnutls_assert();
			return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
		}

		/* Replace old key with new one, and call callback if provided */
		call_rotation_callback(session, key, t);
		session->key.totp.last_result = t;
		memcpy(session->key.session_ticket_key, key, sizeof(key));

		session->key.totp.was_rotated = 1;
	} else if (t < 0) {
		return gnutls_assert_val(t);
	}

	return GNUTLS_E_SUCCESS;
}