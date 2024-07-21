int _gnutls_initialize_session_ticket_key_rotation(gnutls_session_t session, const gnutls_datum_t *key)
{
	if (unlikely(session == NULL || key == NULL))
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	if (session->key.totp.last_result == 0) {
		int64_t t;
		memcpy(session->key.initial_stek, key->data, key->size);
		t = totp_next(session);
		if (t < 0)
			return gnutls_assert_val(t);

		session->key.totp.last_result = t;
		session->key.totp.was_rotated = 0;

		return GNUTLS_E_SUCCESS;
	}

	return GNUTLS_E_INVALID_REQUEST;
}