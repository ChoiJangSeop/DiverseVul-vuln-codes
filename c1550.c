static int crt_to_pubkey(const gnutls_datum_t *raw_crt, gnutls_datum_t * out)
{
gnutls_pubkey_t pub = NULL;
gnutls_x509_crt_t crt = NULL;
int ret;

	out->data = NULL;

	ret = gnutls_x509_crt_init(&crt);
	if (ret < 0)
		return gnutls_assert_val(DANE_E_PUBKEY_ERROR);

	ret = gnutls_pubkey_init( &pub);
	if (ret < 0) {
	        gnutls_assert();
		ret = DANE_E_PUBKEY_ERROR;
		goto cleanup;
	}
		
	ret = gnutls_x509_crt_import(crt, raw_crt, GNUTLS_X509_FMT_DER);
	if (ret < 0) {
	        gnutls_assert();
		ret = DANE_E_PUBKEY_ERROR;
		goto cleanup;
	}

	ret = gnutls_pubkey_import_x509(pub, crt, 0);
	if (ret < 0) {
	        gnutls_assert();
		ret = DANE_E_PUBKEY_ERROR;
		goto cleanup;
	}

	ret = gnutls_pubkey_export2(pub, GNUTLS_X509_FMT_DER, out);
	if (ret < 0) {
	        gnutls_assert();
		ret = DANE_E_PUBKEY_ERROR;
		goto cleanup;
	}
	
	ret = 0;
	goto clean_certs;

cleanup:
	free(out->data);
	out->data = NULL;
clean_certs:
	if (pub)
		gnutls_pubkey_deinit(pub);
	if (crt)
		gnutls_x509_crt_deinit(crt);

	return ret;
}