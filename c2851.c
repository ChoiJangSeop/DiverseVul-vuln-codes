SSL_CTX *tls_init_ctx(fr_tls_server_conf_t *conf, int client)
{
	SSL_CTX		*ctx;
	X509_STORE	*certstore;
	int		verify_mode = SSL_VERIFY_NONE;
	int		ctx_options = 0;
	int		ctx_tls_versions = 0;
	int		type;

	/*
	 *	SHA256 is in all versions of OpenSSL, but isn't
	 *	initialized by default.  It's needed for WiMAX
	 *	certificates.
	 */
#ifdef HAVE_OPENSSL_EVP_SHA256
	EVP_add_digest(EVP_sha256());
#endif

	ctx = SSL_CTX_new(SSLv23_method()); /* which is really "all known SSL / TLS methods".  Idiots. */
	if (!ctx) {
		int err;
		while ((err = ERR_get_error())) {
			ERROR(LOG_PREFIX ": Failed creating SSL context: %s", ERR_error_string(err, NULL));
			return NULL;
		}
	}

	/*
	 * Save the config on the context so that callbacks which
	 * only get SSL_CTX* e.g. session persistence, can get it
	 */
	SSL_CTX_set_app_data(ctx, conf);

	/*
	 * Identify the type of certificates that needs to be loaded
	 */
	if (conf->file_type) {
		type = SSL_FILETYPE_PEM;
	} else {
		type = SSL_FILETYPE_ASN1;
	}

	/*
	 * Set the password to load private key
	 */
	if (conf->private_key_password) {
#ifdef __APPLE__
		/*
		 * We don't want to put the private key password in eap.conf, so  check
		 * for our special string which indicates we should get the password
		 * programmatically.
		 */
		char const* special_string = "Apple:UseCertAdmin";
		if (strncmp(conf->private_key_password, special_string, strlen(special_string)) == 0) {
			char cmd[256];
			char *password;
			long const max_password_len = 128;
			snprintf(cmd, sizeof(cmd) - 1, "/usr/sbin/certadmin --get-private-key-passphrase \"%s\"",
				 conf->private_key_file);

			DEBUG2(LOG_PREFIX ":  Getting private key passphrase using command \"%s\"", cmd);

			FILE* cmd_pipe = popen(cmd, "r");
			if (!cmd_pipe) {
				ERROR(LOG_PREFIX ": %s command failed: Unable to get private_key_password", cmd);
				ERROR(LOG_PREFIX ": Error reading private_key_file %s", conf->private_key_file);
				return NULL;
			}

			rad_const_free(conf->private_key_password);
			password = talloc_array(conf, char, max_password_len);
			if (!password) {
				ERROR(LOG_PREFIX ": Can't allocate space for private_key_password");
				ERROR(LOG_PREFIX ": Error reading private_key_file %s", conf->private_key_file);
				pclose(cmd_pipe);
				return NULL;
			}

			fgets(password, max_password_len, cmd_pipe);
			pclose(cmd_pipe);

			/* Get rid of newline at end of password. */
			password[strlen(password) - 1] = '\0';

			DEBUG3(LOG_PREFIX ": Password from command = \"%s\"", password);
			conf->private_key_password = password;
		}
#endif

		{
			char *password;

			memcpy(&password, &conf->private_key_password, sizeof(password));
			SSL_CTX_set_default_passwd_cb_userdata(ctx, password);
			SSL_CTX_set_default_passwd_cb(ctx, cbtls_password);
		}
	}

#ifdef PSK_MAX_IDENTITY_LEN
	if (!client) {
		/*
		 *	No dynamic query exists.  There MUST be a
		 *	statically configured identity and password.
		 */
		if (conf->psk_query && !*conf->psk_query) {
			ERROR(LOG_PREFIX ": Invalid PSK Configuration: psk_query cannot be empty");
			return NULL;
		}

		/*
		 *	Set the callback only if we can check things.
		 */
		if (conf->psk_identity || conf->psk_query) {
			SSL_CTX_set_psk_server_callback(ctx, psk_server_callback);
		}

	} else if (conf->psk_query) {
		ERROR(LOG_PREFIX ": Invalid PSK Configuration: psk_query cannot be used for outgoing connections");
		return NULL;
	}

	/*
	 *	Now check that if PSK is being used, the config is valid.
	 */
	if ((conf->psk_identity && !conf->psk_password) ||
	    (!conf->psk_identity && conf->psk_password) ||
	    (conf->psk_identity && !*conf->psk_identity) ||
	    (conf->psk_password && !*conf->psk_password)) {
		ERROR(LOG_PREFIX ": Invalid PSK Configuration: psk_identity or psk_password are empty");
		return NULL;
	}

	if (conf->psk_identity) {
		size_t psk_len, hex_len;
		uint8_t buffer[PSK_MAX_PSK_LEN];

		if (conf->certificate_file ||
		    conf->private_key_password || conf->private_key_file ||
		    conf->ca_file || conf->ca_path) {
			ERROR(LOG_PREFIX ": When PSKs are used, No certificate configuration is permitted");
			return NULL;
		}

		if (client) {
			SSL_CTX_set_psk_client_callback(ctx,
							psk_client_callback);
		}

		psk_len = strlen(conf->psk_password);
		if (strlen(conf->psk_password) > (2 * PSK_MAX_PSK_LEN)) {
			ERROR(LOG_PREFIX ": psk_hexphrase is too long (max %d)", PSK_MAX_PSK_LEN);
			return NULL;
		}

		/*
		 *	Check the password now, so that we don't have
		 *	errors at run-time.
		 */
		hex_len = fr_hex2bin(buffer, sizeof(buffer), conf->psk_password, psk_len);
		if (psk_len != (2 * hex_len)) {
			ERROR(LOG_PREFIX ": psk_hexphrase is not all hex");
			return NULL;
		}

		goto post_ca;
	}
#else
	(void) client;	/* -Wunused */
#endif

	/*
	 *	Load our keys and certificates
	 *
	 *	If certificates are of type PEM then we can make use
	 *	of cert chain authentication using openssl api call
	 *	SSL_CTX_use_certificate_chain_file.  Please see how
	 *	the cert chain needs to be given in PEM from
	 *	openSSL.org
	 */
	if (!conf->certificate_file) goto load_ca;

	if (type == SSL_FILETYPE_PEM) {
		if (!(SSL_CTX_use_certificate_chain_file(ctx, conf->certificate_file))) {
			ERROR(LOG_PREFIX ": Error reading certificate file %s:%s", conf->certificate_file,
			      ERR_error_string(ERR_get_error(), NULL));
			return NULL;
		}

	} else if (!(SSL_CTX_use_certificate_file(ctx, conf->certificate_file, type))) {
		ERROR(LOG_PREFIX ": Error reading certificate file %s:%s",
		      conf->certificate_file,
		      ERR_error_string(ERR_get_error(), NULL));
		return NULL;
	}

	/* Load the CAs we trust */
load_ca:
	if (conf->ca_file || conf->ca_path) {
		if (!SSL_CTX_load_verify_locations(ctx, conf->ca_file, conf->ca_path)) {
			ERROR(LOG_PREFIX ": SSL error %s", ERR_error_string(ERR_get_error(), NULL));
			ERROR(LOG_PREFIX ": Error reading Trusted root CA list %s",conf->ca_file );
			return NULL;
		}
	}
	if (conf->ca_file && *conf->ca_file) SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(conf->ca_file));

	if (conf->private_key_file) {
		if (!(SSL_CTX_use_PrivateKey_file(ctx, conf->private_key_file, type))) {
			ERROR(LOG_PREFIX ": Failed reading private key file %s:%s",
			      conf->private_key_file,
			      ERR_error_string(ERR_get_error(), NULL));
			return NULL;
		}

		/*
		 * Check if the loaded private key is the right one
		 */
		if (!SSL_CTX_check_private_key(ctx)) {
			ERROR(LOG_PREFIX ": Private key does not match the certificate public key");
			return NULL;
		}
	}

#ifdef PSK_MAX_IDENTITY_LEN
post_ca:
#endif

	/*
	 *	We never want SSLv2 or SSLv3.
	 */
	ctx_options |= SSL_OP_NO_SSLv2;
	ctx_options |= SSL_OP_NO_SSLv3;

	/*
	 *	As of 3.0.5, we always allow TLSv1.1 and TLSv1.2.
	 *	Though they can be *globally* disabled if necessary.x
	 */
#ifdef SSL_OP_NO_TLSv1
	if (conf->disable_tlsv1) ctx_options |= SSL_OP_NO_TLSv1;

	ctx_tls_versions |= SSL_OP_NO_TLSv1;
#endif
#ifdef SSL_OP_NO_TLSv1_1
	if (conf->disable_tlsv1_1) ctx_options |= SSL_OP_NO_TLSv1_1;

	ctx_tls_versions |= SSL_OP_NO_TLSv1_1;
#endif
#ifdef SSL_OP_NO_TLSv1_2
	if (conf->disable_tlsv1_2) ctx_options |= SSL_OP_NO_TLSv1_2;

	ctx_tls_versions |= SSL_OP_NO_TLSv1_2;
#endif

	if ((ctx_options & ctx_tls_versions) == ctx_tls_versions) {
		ERROR(LOG_PREFIX ": You have disabled all available TLS versions.  EAP will not work");
		return NULL;
	}

#ifdef SSL_OP_NO_TICKET
	ctx_options |= SSL_OP_NO_TICKET;
#endif

	/*
	 *	SSL_OP_SINGLE_DH_USE must be used in order to prevent
	 *	small subgroup attacks and forward secrecy. Always
	 *	using
	 *
	 *	SSL_OP_SINGLE_DH_USE has an impact on the computer
	 *	time needed during negotiation, but it is not very
	 *	large.
	 */
	ctx_options |= SSL_OP_SINGLE_DH_USE;

	/*
	 *	SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS to work around issues
	 *	in Windows Vista client.
	 *	http://www.openssl.org/~bodo/tls-cbc.txt
	 *	http://www.nabble.com/(RADIATOR)-Radiator-Version-3.16-released-t2600070.html
	 */
	ctx_options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;

	SSL_CTX_set_options(ctx, ctx_options);

	/*
	 *	TODO: Set the RSA & DH
	 *	SSL_CTX_set_tmp_rsa_callback(ctx, cbtls_rsa);
	 *	SSL_CTX_set_tmp_dh_callback(ctx, cbtls_dh);
	 */

	/*
	 *	set the message callback to identify the type of
	 *	message.  For every new session, there can be a
	 *	different callback argument.
	 *
	 *	SSL_CTX_set_msg_callback(ctx, cbtls_msg);
	 */

	/*
	 *	Set eliptical curve crypto configuration.
	 */
#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#ifndef OPENSSL_NO_ECDH
	if (set_ecdh_curve(ctx, conf->ecdh_curve) < 0) {
		return NULL;
	}
#endif
#endif

	/* Set Info callback */
	SSL_CTX_set_info_callback(ctx, cbtls_info);

	/*
	 *	Callbacks, etc. for session resumption.
	 */
	if (conf->session_cache_enable) {
		/*
		 *	Cache sessions on disk if requested.
		 */
		if (conf->session_cache_path) {
			SSL_CTX_sess_set_new_cb(ctx, cbtls_new_session);
			SSL_CTX_sess_set_get_cb(ctx, cbtls_get_session);
			SSL_CTX_sess_set_remove_cb(ctx, cbtls_remove_session);
		}

		SSL_CTX_set_quiet_shutdown(ctx, 1);
		if (fr_tls_ex_index_vps < 0)
			fr_tls_ex_index_vps = SSL_SESSION_get_ex_new_index(0, NULL, NULL, NULL, sess_free_vps);
	}

	/*
	 *	Check the certificates for revocation.
	 */
#ifdef X509_V_FLAG_CRL_CHECK
	if (conf->check_crl) {
		certstore = SSL_CTX_get_cert_store(ctx);
		if (certstore == NULL) {
			ERROR(LOG_PREFIX ": SSL error %s", ERR_error_string(ERR_get_error(), NULL));
			ERROR(LOG_PREFIX ": Error reading Certificate Store");
	    		return NULL;
		}
		X509_STORE_set_flags(certstore, X509_V_FLAG_CRL_CHECK);
	}
#endif

	/*
	 *	Set verify modes
	 *	Always verify the peer certificate
	 */
	verify_mode |= SSL_VERIFY_PEER;
	verify_mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
	verify_mode |= SSL_VERIFY_CLIENT_ONCE;
	SSL_CTX_set_verify(ctx, verify_mode, cbtls_verify);

	if (conf->verify_depth) {
		SSL_CTX_set_verify_depth(ctx, conf->verify_depth);
	}

	/* Load randomness */
	if (conf->random_file) {
		if (!(RAND_load_file(conf->random_file, 1024*10))) {
			ERROR(LOG_PREFIX ": SSL error %s", ERR_error_string(ERR_get_error(), NULL));
			ERROR(LOG_PREFIX ": Error loading randomness");
			return NULL;
		}
	}

	/*
	 * Set the cipher list if we were told to
	 */
	if (conf->cipher_list) {
		if (!SSL_CTX_set_cipher_list(ctx, conf->cipher_list)) {
			ERROR(LOG_PREFIX ": Error setting cipher list");
			return NULL;
		}
	}

	/*
	 *	Setup session caching
	 */
	if (conf->session_cache_enable) {
		/*
		 *	Create a unique context Id per EAP-TLS configuration.
		 */
		if (conf->session_id_name) {
			snprintf(conf->session_context_id, sizeof(conf->session_context_id),
				 "FR eap %s", conf->session_id_name);
		} else {
			snprintf(conf->session_context_id, sizeof(conf->session_context_id),
				 "FR eap %p", conf);
		}

		/*
		 *	Cache it, and DON'T auto-clear it.
		 */
		SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_AUTO_CLEAR);

		SSL_CTX_set_session_id_context(ctx,
					       (unsigned char *) conf->session_context_id,
					       (unsigned int) strlen(conf->session_context_id));

		/*
		 *	Our timeout is in hours, this is in seconds.
		 */
		SSL_CTX_set_timeout(ctx, conf->session_timeout * 3600);

		/*
		 *	Set the maximum number of entries in the
		 *	session cache.
		 */
		SSL_CTX_sess_set_cache_size(ctx, conf->session_cache_size);

	} else {
		SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);
	}

	return ctx;
}