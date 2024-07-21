void tls_cert_generate(TALLOC_CTX *mem_ctx, 
		       const char *hostname, 
		       const char *keyfile, const char *certfile,
		       const char *cafile)
{
	gnutls_x509_crt cacrt, crt;
	gnutls_x509_privkey key, cakey;
	uint32_t serial = (uint32_t)time(NULL);
	unsigned char keyid[100];
	char buf[4096];
	size_t bufsize;
	size_t keyidsize = sizeof(keyid);
	time_t activation = time(NULL), expiry = activation + LIFETIME;
	int ret;

	if (file_exist(keyfile) || file_exist(certfile) || file_exist(cafile)) {
		DEBUG(0,("TLS autogeneration skipped - some TLS files already exist\n"));
		return;
	}

#define TLSCHECK(call) do { \
	ret = call; \
	if (ret < 0) { \
		DEBUG(0,("TLS %s - %s\n", #call, gnutls_strerror(ret))); \
		goto failed; \
	} \
} while (0)

	TLSCHECK(gnutls_global_init());

	DEBUG(0,("Attempting to autogenerate TLS self-signed keys for https for hostname '%s'\n", 
		 hostname));
	
#ifdef HAVE_GCRYPT_H
	DEBUG(3,("Enabling QUICK mode in gcrypt\n"));
	gcry_control(GCRYCTL_ENABLE_QUICK_RANDOM, 0);
#endif

	DEBUG(3,("Generating private key\n"));
	TLSCHECK(gnutls_x509_privkey_init(&key));
	TLSCHECK(gnutls_x509_privkey_generate(key,   GNUTLS_PK_RSA, DH_BITS, 0));

	DEBUG(3,("Generating CA private key\n"));
	TLSCHECK(gnutls_x509_privkey_init(&cakey));
	TLSCHECK(gnutls_x509_privkey_generate(cakey, GNUTLS_PK_RSA, DH_BITS, 0));

	DEBUG(3,("Generating CA certificate\n"));
	TLSCHECK(gnutls_x509_crt_init(&cacrt));
	TLSCHECK(gnutls_x509_crt_set_dn_by_oid(cacrt, 
				      GNUTLS_OID_X520_ORGANIZATION_NAME, 0,
				      ORGANISATION_NAME, strlen(ORGANISATION_NAME)));
	TLSCHECK(gnutls_x509_crt_set_dn_by_oid(cacrt, 
				      GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME, 0,
				      UNIT_NAME, strlen(UNIT_NAME)));
	TLSCHECK(gnutls_x509_crt_set_dn_by_oid(cacrt,
				      GNUTLS_OID_X520_COMMON_NAME, 0,
				      hostname, strlen(hostname)));
	TLSCHECK(gnutls_x509_crt_set_key(cacrt, cakey));
	TLSCHECK(gnutls_x509_crt_set_serial(cacrt, &serial, sizeof(serial)));
	TLSCHECK(gnutls_x509_crt_set_activation_time(cacrt, activation));
	TLSCHECK(gnutls_x509_crt_set_expiration_time(cacrt, expiry));
	TLSCHECK(gnutls_x509_crt_set_ca_status(cacrt, 0));
#ifdef GNUTLS_KP_TLS_WWW_SERVER
	TLSCHECK(gnutls_x509_crt_set_key_purpose_oid(cacrt, GNUTLS_KP_TLS_WWW_SERVER, 0));
#endif
	TLSCHECK(gnutls_x509_crt_set_version(cacrt, 3));
	TLSCHECK(gnutls_x509_crt_get_key_id(cacrt, 0, keyid, &keyidsize));
#if HAVE_GNUTLS_X509_CRT_SET_SUBJECT_KEY_ID
	TLSCHECK(gnutls_x509_crt_set_subject_key_id(cacrt, keyid, keyidsize));
#endif
	TLSCHECK(gnutls_x509_crt_sign(cacrt, cacrt, cakey));

	DEBUG(3,("Generating TLS certificate\n"));
	TLSCHECK(gnutls_x509_crt_init(&crt));
	TLSCHECK(gnutls_x509_crt_set_dn_by_oid(crt, 
				      GNUTLS_OID_X520_ORGANIZATION_NAME, 0,
				      ORGANISATION_NAME, strlen(ORGANISATION_NAME)));
	TLSCHECK(gnutls_x509_crt_set_dn_by_oid(crt, 
				      GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME, 0,
				      UNIT_NAME, strlen(UNIT_NAME)));
	TLSCHECK(gnutls_x509_crt_set_dn_by_oid(crt,
				      GNUTLS_OID_X520_COMMON_NAME, 0,
				      hostname, strlen(hostname)));
	TLSCHECK(gnutls_x509_crt_set_key(crt, key));
	TLSCHECK(gnutls_x509_crt_set_serial(crt, &serial, sizeof(serial)));
	TLSCHECK(gnutls_x509_crt_set_activation_time(crt, activation));
	TLSCHECK(gnutls_x509_crt_set_expiration_time(crt, expiry));
	TLSCHECK(gnutls_x509_crt_set_ca_status(crt, 0));
#ifdef GNUTLS_KP_TLS_WWW_SERVER
	TLSCHECK(gnutls_x509_crt_set_key_purpose_oid(crt, GNUTLS_KP_TLS_WWW_SERVER, 0));
#endif
	TLSCHECK(gnutls_x509_crt_set_version(crt, 3));
	TLSCHECK(gnutls_x509_crt_get_key_id(crt, 0, keyid, &keyidsize));
#if HAVE_GNUTLS_X509_CRT_SET_SUBJECT_KEY_ID
	TLSCHECK(gnutls_x509_crt_set_subject_key_id(crt, keyid, keyidsize));
#endif
	TLSCHECK(gnutls_x509_crt_sign(crt, crt, key));

	DEBUG(3,("Exporting TLS keys\n"));

	bufsize = sizeof(buf);
	TLSCHECK(gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_PEM, buf, &bufsize));
	if (!file_save(certfile, buf, bufsize)) {
		DEBUG(0,("Unable to save certificate in %s parent dir exists ?\n", certfile));
		goto failed;
	}

	bufsize = sizeof(buf);
	TLSCHECK(gnutls_x509_crt_export(cacrt, GNUTLS_X509_FMT_PEM, buf, &bufsize));
	if (!file_save(cafile, buf, bufsize)) {
		DEBUG(0,("Unable to save ca cert in %s parent dir exists ?\n", cafile));
		goto failed;
	}

	bufsize = sizeof(buf);
	TLSCHECK(gnutls_x509_privkey_export(key, GNUTLS_X509_FMT_PEM, buf, &bufsize));
	if (!file_save(keyfile, buf, bufsize)) {
		DEBUG(0,("Unable to save privatekey in %s parent dir exists ?\n", keyfile));
		goto failed;
	}

	gnutls_x509_privkey_deinit(key);
	gnutls_x509_privkey_deinit(cakey);
	gnutls_x509_crt_deinit(cacrt);
	gnutls_x509_crt_deinit(crt);
	gnutls_global_deinit();

	DEBUG(0,("TLS self-signed keys generated OK\n"));
	return;

failed:
	DEBUG(0,("TLS certificate generation failed\n"));
}