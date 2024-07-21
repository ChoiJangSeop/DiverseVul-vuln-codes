PHP_FUNCTION(openssl_pkcs7_decrypt)
{
	zval ** recipcert, ** recipkey = NULL;
	X509 * cert = NULL;
	EVP_PKEY * key = NULL;
	long certresval, keyresval;
	BIO * in = NULL, * out = NULL, * datain = NULL;
	PKCS7 * p7 = NULL;
	char * infilename;	int infilename_len;
	char * outfilename;	int outfilename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssZ|Z", &infilename, &infilename_len,
				&outfilename, &outfilename_len, &recipcert, &recipkey) == FAILURE) {
		return;
	}

	RETVAL_FALSE;

	cert = php_openssl_x509_from_zval(recipcert, 0, &certresval TSRMLS_CC);
	if (cert == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to coerce parameter 3 to x509 cert");
		goto clean_exit;
	}

	key = php_openssl_evp_from_zval(recipkey ? recipkey : recipcert, 0, "", 0, &keyresval TSRMLS_CC);
	if (key == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to get private key");
		goto clean_exit;
	}
	
	if (php_openssl_safe_mode_chk(infilename TSRMLS_CC) || php_openssl_safe_mode_chk(outfilename TSRMLS_CC)) {
		goto clean_exit;
	}

	in = BIO_new_file(infilename, "r");
	if (in == NULL) {
		goto clean_exit;
	}
	out = BIO_new_file(outfilename, "w");
	if (out == NULL) {
		goto clean_exit;
	}

	p7 = SMIME_read_PKCS7(in, &datain);

	if (p7 == NULL) {
		goto clean_exit;
	}
	if (PKCS7_decrypt(p7, key, cert, out, PKCS7_DETACHED)) { 
		RETVAL_TRUE;
	}
clean_exit:
	PKCS7_free(p7);
	BIO_free(datain);
	BIO_free(in);
	BIO_free(out);
	if (cert && certresval == -1) {
		X509_free(cert);
	}
	if (key && keyresval == -1) {
		EVP_PKEY_free(key);
	}
}