static int verify_ca(const gnutls_datum_t *raw_crt, unsigned raw_crt_size,
					 gnutls_certificate_type_t crt_type,
					 dane_cert_type_t ctype,
					 dane_match_type_t match, gnutls_datum_t * data,
					 unsigned int *verify)
{
gnutls_datum_t pubkey = {NULL, 0};
int ret;
unsigned int vstatus;
gnutls_x509_crt_t crt = NULL, ca = NULL;

	if (raw_crt_size < 2)
		return gnutls_assert_val(DANE_E_INVALID_REQUEST);

	if (ctype == DANE_CERT_X509 && crt_type == GNUTLS_CRT_X509) {
	
		if (!matches(&raw_crt[1], data, match)) {
		        gnutls_assert();
			*verify |= DANE_VERIFY_CA_CONSTRAINTS_VIOLATED;
                }

	} else if (ctype == DANE_CERT_PK && crt_type == GNUTLS_CRT_X509) {
		ret = crt_to_pubkey(&raw_crt[1], &pubkey);
		if (ret < 0) {
        	        gnutls_assert();
			goto cleanup;
                }

		if (!matches(&pubkey, data, match)) {
                        gnutls_assert();
			*verify |= DANE_VERIFY_CA_CONSTRAINTS_VIOLATED;
                }
	} else {
		ret = gnutls_assert_val(DANE_E_UNKNOWN_DANE_DATA);
		goto cleanup;
	}
	
	/* check if the certificate chain is actually a chain */
	ret = gnutls_x509_crt_init(&crt);
  	if (ret < 0) {
  	  	ret = gnutls_assert_val(DANE_E_CERT_ERROR);
  	  	goto cleanup;
	}

	ret = gnutls_x509_crt_init(&ca);
  	if (ret < 0) {
  	  	ret = gnutls_assert_val(DANE_E_CERT_ERROR);
  	  	goto cleanup;
	}

	ret = gnutls_x509_crt_import(crt, &raw_crt[0], GNUTLS_X509_FMT_DER);
  	if (ret < 0) {
  	  	ret = gnutls_assert_val(DANE_E_CERT_ERROR);
  	  	goto cleanup;
	}

	ret = gnutls_x509_crt_import(ca, &raw_crt[1], GNUTLS_X509_FMT_DER);
  	if (ret < 0) {
  	  	ret = gnutls_assert_val(DANE_E_CERT_ERROR);
  	  	goto cleanup;
	}
	    
	ret = gnutls_x509_crt_check_issuer(crt, ca);
	if (ret == 0) {
		gnutls_assert();
		*verify |= DANE_VERIFY_CA_CONSTRAINTS_VIOLATED;
	}

	ret = gnutls_x509_crt_verify(crt, &ca, 1, 0, &vstatus);
	if (ret < 0) {
  	  	ret = gnutls_assert_val(DANE_E_CERT_ERROR);
  	  	goto cleanup;
	}
	if (vstatus != 0)
		*verify |= DANE_VERIFY_CA_CONSTRAINTS_VIOLATED;

	ret = 0;
cleanup:
	free(pubkey.data);
	if (crt != NULL)
	  gnutls_x509_crt_deinit(crt);
	if (ca != NULL)
	  gnutls_x509_crt_deinit(ca);
	return ret;
}