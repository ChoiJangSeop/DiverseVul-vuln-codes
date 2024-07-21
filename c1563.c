struct tls_params *tls_initialise(TALLOC_CTX *mem_ctx, struct loadparm_context *lp_ctx)
{
	struct tls_params *params;
	int ret;
	TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);
	const char *keyfile = lpcfg_tls_keyfile(tmp_ctx, lp_ctx);
	const char *certfile = lpcfg_tls_certfile(tmp_ctx, lp_ctx);
	const char *cafile = lpcfg_tls_cafile(tmp_ctx, lp_ctx);
	const char *crlfile = lpcfg_tls_crlfile(tmp_ctx, lp_ctx);
	const char *dhpfile = lpcfg_tls_dhpfile(tmp_ctx, lp_ctx);
	void tls_cert_generate(TALLOC_CTX *, const char *, const char *, const char *, const char *);
	params = talloc(mem_ctx, struct tls_params);
	if (params == NULL) {
		talloc_free(tmp_ctx);
		return NULL;
	}

	if (!lpcfg_tls_enabled(lp_ctx) || keyfile == NULL || *keyfile == 0) {
		params->tls_enabled = false;
		talloc_free(tmp_ctx);
		return params;
	}

	if (!file_exist(cafile)) {
		char *hostname = talloc_asprintf(mem_ctx, "%s.%s",
						 lpcfg_netbios_name(lp_ctx),
						 lpcfg_dnsdomain(lp_ctx));
		if (hostname == NULL) {
			goto init_failed;
		}
		tls_cert_generate(params, hostname, keyfile, certfile, cafile);
		talloc_free(hostname);
	}

	ret = gnutls_global_init();
	if (ret < 0) goto init_failed;

	gnutls_certificate_allocate_credentials(&params->x509_cred);
	if (ret < 0) goto init_failed;

	if (cafile && *cafile) {
		ret = gnutls_certificate_set_x509_trust_file(params->x509_cred, cafile, 
							     GNUTLS_X509_FMT_PEM);	
		if (ret < 0) {
			DEBUG(0,("TLS failed to initialise cafile %s\n", cafile));
			goto init_failed;
		}
	}

	if (crlfile && *crlfile) {
		ret = gnutls_certificate_set_x509_crl_file(params->x509_cred, 
							   crlfile, 
							   GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			DEBUG(0,("TLS failed to initialise crlfile %s\n", crlfile));
			goto init_failed;
		}
	}
	
	ret = gnutls_certificate_set_x509_key_file(params->x509_cred, 
						   certfile, keyfile,
						   GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		DEBUG(0,("TLS failed to initialise certfile %s and keyfile %s\n", 
			 certfile, keyfile));
		goto init_failed;
	}
	
	
	ret = gnutls_dh_params_init(&params->dh_params);
	if (ret < 0) goto init_failed;

	if (dhpfile && *dhpfile) {
		gnutls_datum_t dhparms;
		size_t size;
		dhparms.data = (uint8_t *)file_load(dhpfile, &size, 0, mem_ctx);

		if (!dhparms.data) {
			DEBUG(0,("Failed to read DH Parms from %s\n", dhpfile));
			goto init_failed;
		}
		dhparms.size = size;
			
		ret = gnutls_dh_params_import_pkcs3(params->dh_params, &dhparms, GNUTLS_X509_FMT_PEM);
		if (ret < 0) goto init_failed;
	} else {
		ret = gnutls_dh_params_generate2(params->dh_params, DH_BITS);
		if (ret < 0) goto init_failed;
	}
		
	gnutls_certificate_set_dh_params(params->x509_cred, params->dh_params);

	params->tls_enabled = true;

	talloc_free(tmp_ctx);
	return params;

init_failed:
	DEBUG(0,("GNUTLS failed to initialise - %s\n", gnutls_strerror(ret)));
	params->tls_enabled = false;
	talloc_free(tmp_ctx);
	return params;
}