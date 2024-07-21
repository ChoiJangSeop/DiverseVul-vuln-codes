NTSTATUS tstream_tls_params_server(TALLOC_CTX *mem_ctx,
				   const char *dns_host_name,
				   bool enabled,
				   const char *key_file,
				   const char *cert_file,
				   const char *ca_file,
				   const char *crl_file,
				   const char *dhp_file,
				   struct tstream_tls_params **_tlsp)
{
	struct tstream_tls_params *tlsp;
#if ENABLE_GNUTLS
	int ret;

	if (!enabled || key_file == NULL || *key_file == 0) {
		tlsp = talloc_zero(mem_ctx, struct tstream_tls_params);
		NT_STATUS_HAVE_NO_MEMORY(tlsp);
		talloc_set_destructor(tlsp, tstream_tls_params_destructor);
		tlsp->tls_enabled = false;

		*_tlsp = tlsp;
		return NT_STATUS_OK;
	}

	ret = gnutls_global_init();
	if (ret != GNUTLS_E_SUCCESS) {
		DEBUG(0,("TLS %s - %s\n", __location__, gnutls_strerror(ret)));
		return NT_STATUS_NOT_SUPPORTED;
	}

	tlsp = talloc_zero(mem_ctx, struct tstream_tls_params);
	NT_STATUS_HAVE_NO_MEMORY(tlsp);

	talloc_set_destructor(tlsp, tstream_tls_params_destructor);

	if (!file_exist(ca_file)) {
		tls_cert_generate(tlsp, dns_host_name,
				  key_file, cert_file, ca_file);
	}

	ret = gnutls_certificate_allocate_credentials(&tlsp->x509_cred);
	if (ret != GNUTLS_E_SUCCESS) {
		DEBUG(0,("TLS %s - %s\n", __location__, gnutls_strerror(ret)));
		talloc_free(tlsp);
		return NT_STATUS_NO_MEMORY;
	}

	if (ca_file && *ca_file) {
		ret = gnutls_certificate_set_x509_trust_file(tlsp->x509_cred,
							     ca_file,
							     GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			DEBUG(0,("TLS failed to initialise cafile %s - %s\n",
				 ca_file, gnutls_strerror(ret)));
			talloc_free(tlsp);
			return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
		}
	}

	if (crl_file && *crl_file) {
		ret = gnutls_certificate_set_x509_crl_file(tlsp->x509_cred,
							   crl_file, 
							   GNUTLS_X509_FMT_PEM);
		if (ret < 0) {
			DEBUG(0,("TLS failed to initialise crlfile %s - %s\n",
				 crl_file, gnutls_strerror(ret)));
			talloc_free(tlsp);
			return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
		}
	}

	ret = gnutls_certificate_set_x509_key_file(tlsp->x509_cred,
						   cert_file, key_file,
						   GNUTLS_X509_FMT_PEM);
	if (ret != GNUTLS_E_SUCCESS) {
		DEBUG(0,("TLS failed to initialise certfile %s and keyfile %s - %s\n",
			 cert_file, key_file, gnutls_strerror(ret)));
		talloc_free(tlsp);
		return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
	}

	ret = gnutls_dh_params_init(&tlsp->dh_params);
	if (ret != GNUTLS_E_SUCCESS) {
		DEBUG(0,("TLS %s - %s\n", __location__, gnutls_strerror(ret)));
		talloc_free(tlsp);
		return NT_STATUS_NO_MEMORY;
	}

	if (dhp_file && *dhp_file) {
		gnutls_datum_t dhparms;
		size_t size;

		dhparms.data = (uint8_t *)file_load(dhp_file, &size, 0, tlsp);

		if (!dhparms.data) {
			DEBUG(0,("TLS failed to read DH Parms from %s - %d:%s\n",
				 dhp_file, errno, strerror(errno)));
			talloc_free(tlsp);
			return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
		}
		dhparms.size = size;

		ret = gnutls_dh_params_import_pkcs3(tlsp->dh_params,
						    &dhparms,
						    GNUTLS_X509_FMT_PEM);
		if (ret != GNUTLS_E_SUCCESS) {
			DEBUG(0,("TLS failed to import pkcs3 %s - %s\n",
				 dhp_file, gnutls_strerror(ret)));
			talloc_free(tlsp);
			return NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
		}
	} else {
		ret = gnutls_dh_params_generate2(tlsp->dh_params, DH_BITS);
		if (ret != GNUTLS_E_SUCCESS) {
			DEBUG(0,("TLS failed to generate dh_params - %s\n",
				 gnutls_strerror(ret)));
			talloc_free(tlsp);
			return NT_STATUS_INTERNAL_ERROR;
		}
	}

	gnutls_certificate_set_dh_params(tlsp->x509_cred, tlsp->dh_params);

	tlsp->tls_enabled = true;

#else /* ENABLE_GNUTLS */
	tlsp = talloc_zero(mem_ctx, struct tstream_tls_params);
	NT_STATUS_HAVE_NO_MEMORY(tlsp);
	talloc_set_destructor(tlsp, tstream_tls_params_destructor);
	tlsp->tls_enabled = false;
#endif /* ENABLE_GNUTLS */

	*_tlsp = tlsp;
	return NT_STATUS_OK;
}