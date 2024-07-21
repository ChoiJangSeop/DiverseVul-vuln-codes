PHP_FUNCTION(openssl_pkcs12_export_to_file)
{
	X509 * cert = NULL;
	BIO * bio_out = NULL;
	PKCS12 * p12 = NULL;
	char * filename;
	char * friendly_name = NULL;
	int filename_len;
	char * pass;
	int pass_len;
	zval **zcert = NULL, *zpkey = NULL, *args = NULL;
	EVP_PKEY *priv_key = NULL;
	long certresource, keyresource;
	zval ** item;
	STACK_OF(X509) *ca = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Zszs|a", &zcert, &filename, &filename_len, &zpkey, &pass, &pass_len, &args) == FAILURE)
		return;

	RETVAL_FALSE;
	
	cert = php_openssl_x509_from_zval(zcert, 0, &certresource TSRMLS_CC);
	if (cert == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot get cert from parameter 1");
		return;
	}
	priv_key = php_openssl_evp_from_zval(&zpkey, 0, "", 1, &keyresource TSRMLS_CC);
	if (priv_key == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot get private key from parameter 3");
		goto cleanup;
	}
	if (cert && !X509_check_private_key(cert, priv_key)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "private key does not correspond to cert");
		goto cleanup;
	}
	if (php_openssl_safe_mode_chk(filename TSRMLS_CC)) {
		goto cleanup;
	}

	/* parse extra config from args array, promote this to an extra function */
	if (args && zend_hash_find(Z_ARRVAL_P(args), "friendly_name", sizeof("friendly_name"), (void**)&item) == SUCCESS)
		friendly_name = Z_STRVAL_PP(item);
	/* certpbe (default RC2-40)
	   keypbe (default 3DES)
	   friendly_caname
	*/

	if (args && zend_hash_find(Z_ARRVAL_P(args), "extracerts", sizeof("extracerts"), (void**)&item) == SUCCESS)
		ca = php_array_to_X509_sk(item TSRMLS_CC);
	/* end parse extra config */

	/*PKCS12 *PKCS12_create(char *pass, char *name, EVP_PKEY *pkey, X509 *cert, STACK_OF(X509) *ca,
                                       int nid_key, int nid_cert, int iter, int mac_iter, int keytype);*/

	p12 = PKCS12_create(pass, friendly_name, priv_key, cert, ca, 0, 0, 0, 0, 0);

	bio_out = BIO_new_file(filename, "w"); 
	if (bio_out) {
		
		i2d_PKCS12_bio(bio_out, p12);

		RETVAL_TRUE;
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "error opening file %s", filename);
	}

	BIO_free(bio_out);
	PKCS12_free(p12);
	php_sk_X509_free(ca);
	
cleanup:

	if (keyresource == -1 && priv_key) {
		EVP_PKEY_free(priv_key);
	}
	if (certresource == -1 && cert) { 
		X509_free(cert);
	}
}