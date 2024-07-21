NTSTATUS smb2cli_session_set_session_key(struct smbXcli_session *session,
					 const DATA_BLOB _session_key,
					 const struct iovec *recv_iov)
{
	struct smbXcli_conn *conn = session->conn;
	uint16_t no_sign_flags;
	uint8_t session_key[16];
	bool check_signature = true;
	uint32_t hdr_flags;
	NTSTATUS status;
	struct _derivation {
		DATA_BLOB label;
		DATA_BLOB context;
	};
	struct {
		struct _derivation signing;
		struct _derivation encryption;
		struct _derivation decryption;
		struct _derivation application;
	} derivation = { };
	size_t nonce_size = 0;

	if (conn == NULL) {
		return NT_STATUS_INVALID_PARAMETER_MIX;
	}

	if (recv_iov[0].iov_len != SMB2_HDR_BODY) {
		return NT_STATUS_INVALID_PARAMETER_MIX;
	}

	no_sign_flags = SMB2_SESSION_FLAG_IS_GUEST | SMB2_SESSION_FLAG_IS_NULL;

	if (session->smb2->session_flags & no_sign_flags) {
		session->smb2->should_sign = false;
		return NT_STATUS_OK;
	}

	if (session->smb2->signing_key.length != 0) {
		return NT_STATUS_INVALID_PARAMETER_MIX;
	}

	if (conn->protocol >= PROTOCOL_SMB3_10) {
		struct _derivation *d;
		DATA_BLOB p;

		p = data_blob_const(session->smb2_channel.preauth_sha512,
				sizeof(session->smb2_channel.preauth_sha512));

		d = &derivation.signing;
		d->label = data_blob_string_const_null("SMBSigningKey");
		d->context = p;

		d = &derivation.encryption;
		d->label = data_blob_string_const_null("SMBC2SCipherKey");
		d->context = p;

		d = &derivation.decryption;
		d->label = data_blob_string_const_null("SMBS2CCipherKey");
		d->context = p;

		d = &derivation.application;
		d->label = data_blob_string_const_null("SMBAppKey");
		d->context = p;

	} else if (conn->protocol >= PROTOCOL_SMB2_24) {
		struct _derivation *d;

		d = &derivation.signing;
		d->label = data_blob_string_const_null("SMB2AESCMAC");
		d->context = data_blob_string_const_null("SmbSign");

		d = &derivation.encryption;
		d->label = data_blob_string_const_null("SMB2AESCCM");
		d->context = data_blob_string_const_null("ServerIn ");

		d = &derivation.decryption;
		d->label = data_blob_string_const_null("SMB2AESCCM");
		d->context = data_blob_string_const_null("ServerOut");

		d = &derivation.application;
		d->label = data_blob_string_const_null("SMB2APP");
		d->context = data_blob_string_const_null("SmbRpc");
	}

	ZERO_STRUCT(session_key);
	memcpy(session_key, _session_key.data,
	       MIN(_session_key.length, sizeof(session_key)));

	session->smb2->signing_key = data_blob_talloc(session,
						     session_key,
						     sizeof(session_key));
	if (session->smb2->signing_key.data == NULL) {
		ZERO_STRUCT(session_key);
		return NT_STATUS_NO_MEMORY;
	}

	if (conn->protocol >= PROTOCOL_SMB2_24) {
		struct _derivation *d = &derivation.signing;

		smb2_key_derivation(session_key, sizeof(session_key),
				    d->label.data, d->label.length,
				    d->context.data, d->context.length,
				    session->smb2->signing_key.data);
	}

	session->smb2->encryption_key = data_blob_dup_talloc(session,
						session->smb2->signing_key);
	if (session->smb2->encryption_key.data == NULL) {
		ZERO_STRUCT(session_key);
		return NT_STATUS_NO_MEMORY;
	}

	if (conn->protocol >= PROTOCOL_SMB2_24) {
		struct _derivation *d = &derivation.encryption;

		smb2_key_derivation(session_key, sizeof(session_key),
				    d->label.data, d->label.length,
				    d->context.data, d->context.length,
				    session->smb2->encryption_key.data);
	}

	session->smb2->decryption_key = data_blob_dup_talloc(session,
						session->smb2->signing_key);
	if (session->smb2->decryption_key.data == NULL) {
		ZERO_STRUCT(session_key);
		return NT_STATUS_NO_MEMORY;
	}

	if (conn->protocol >= PROTOCOL_SMB2_24) {
		struct _derivation *d = &derivation.decryption;

		smb2_key_derivation(session_key, sizeof(session_key),
				    d->label.data, d->label.length,
				    d->context.data, d->context.length,
				    session->smb2->decryption_key.data);
	}

	session->smb2->application_key = data_blob_dup_talloc(session,
						session->smb2->signing_key);
	if (session->smb2->application_key.data == NULL) {
		ZERO_STRUCT(session_key);
		return NT_STATUS_NO_MEMORY;
	}

	if (conn->protocol >= PROTOCOL_SMB2_24) {
		struct _derivation *d = &derivation.application;

		smb2_key_derivation(session_key, sizeof(session_key),
				    d->label.data, d->label.length,
				    d->context.data, d->context.length,
				    session->smb2->application_key.data);
	}
	ZERO_STRUCT(session_key);

	session->smb2_channel.signing_key = data_blob_dup_talloc(session,
						session->smb2->signing_key);
	if (session->smb2_channel.signing_key.data == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	check_signature = conn->mandatory_signing;

	hdr_flags = IVAL(recv_iov[0].iov_base, SMB2_HDR_FLAGS);
	if (hdr_flags & SMB2_HDR_FLAG_SIGNED) {
		/*
		 * Sadly some vendors don't sign the
		 * final SMB2 session setup response
		 *
		 * At least Windows and Samba are always doing this
		 * if there's a session key available.
		 *
		 * We only check the signature if it's mandatory
		 * or SMB2_HDR_FLAG_SIGNED is provided.
		 */
		check_signature = true;
	}

	if (conn->protocol >= PROTOCOL_SMB3_10) {
		check_signature = true;
	}

	if (check_signature) {
		status = smb2_signing_check_pdu(session->smb2_channel.signing_key,
						session->conn->protocol,
						recv_iov, 3);
		if (!NT_STATUS_IS_OK(status)) {
			return status;
		}
	}

	session->smb2->should_sign = false;
	session->smb2->should_encrypt = false;

	if (conn->desire_signing) {
		session->smb2->should_sign = true;
	}

	if (conn->smb2.server.security_mode & SMB2_NEGOTIATE_SIGNING_REQUIRED) {
		session->smb2->should_sign = true;
	}

	if (session->smb2->session_flags & SMB2_SESSION_FLAG_ENCRYPT_DATA) {
		session->smb2->should_encrypt = true;
	}

	if (conn->protocol < PROTOCOL_SMB2_24) {
		session->smb2->should_encrypt = false;
	}

	if (conn->smb2.server.cipher == 0) {
		session->smb2->should_encrypt = false;
	}

	/*
	 * CCM and GCM algorithms must never have their
	 * nonce wrap, or the security of the whole
	 * communication and the keys is destroyed.
	 * We must drop the connection once we have
	 * transfered too much data.
	 *
	 * NOTE: We assume nonces greater than 8 bytes.
	 */
	generate_random_buffer((uint8_t *)&session->smb2->nonce_high_random,
			       sizeof(session->smb2->nonce_high_random));
	switch (conn->smb2.server.cipher) {
	case SMB2_ENCRYPTION_AES128_CCM:
		nonce_size = AES_CCM_128_NONCE_SIZE;
		break;
	case SMB2_ENCRYPTION_AES128_GCM:
		nonce_size = AES_GCM_128_IV_SIZE;
		break;
	default:
		nonce_size = 0;
		break;
	}
	session->smb2->nonce_high_max = SMB2_NONCE_HIGH_MAX(nonce_size);
	session->smb2->nonce_high = 0;
	session->smb2->nonce_low = 0;

	return NT_STATUS_OK;
}