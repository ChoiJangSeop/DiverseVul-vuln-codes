PHP_FUNCTION(openssl_pkey_export_to_file)
{
	struct php_x509_request req;
	zval ** zpkey, * args = NULL;
	char * passphrase = NULL; int passphrase_len = 0;
	char * filename = NULL; int filename_len = 0;
	long key_resource = -1;
	EVP_PKEY * key;
	BIO * bio_out = NULL;
	const EVP_CIPHER * cipher;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Zs|s!a!", &zpkey, &filename, &filename_len, &passphrase, &passphrase_len, &args) == FAILURE) {
		return;
	}
	RETVAL_FALSE;

	key = php_openssl_evp_from_zval(zpkey, 0, passphrase, 0, &key_resource TSRMLS_CC);

	if (key == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot get key from parameter 1");
		RETURN_FALSE;
	}
	
	if (php_openssl_safe_mode_chk(filename TSRMLS_CC)) {
		RETURN_FALSE;
	}
	
	PHP_SSL_REQ_INIT(&req);

	if (PHP_SSL_REQ_PARSE(&req, args) == SUCCESS) {
		bio_out = BIO_new_file(filename, "w");

		if (passphrase && req.priv_key_encrypt) {
			cipher = (EVP_CIPHER *) EVP_des_ede3_cbc();
		} else {
			cipher = NULL;
		}
		if (PEM_write_bio_PrivateKey(bio_out, key, cipher, (unsigned char *)passphrase, passphrase_len, NULL, NULL)) {
			/* Success!
			 * If returning the output as a string, do so now */
			RETVAL_TRUE;
		}
	}
	PHP_SSL_REQ_DISPOSE(&req);

	if (key_resource == -1 && key) {
		EVP_PKEY_free(key);
	}
	if (bio_out) {
		BIO_free(bio_out);
	}
}