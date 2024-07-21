PHP_FUNCTION(openssl_pkcs7_sign)
{
	zval ** zcert, ** zprivkey, * zheaders;
	zval ** hval;
	X509 * cert = NULL;
	EVP_PKEY * privkey = NULL;
	long flags = PKCS7_DETACHED;
	PKCS7 * p7 = NULL;
	BIO * infile = NULL, * outfile = NULL;
	STACK_OF(X509) *others = NULL;
	long certresource = -1, keyresource = -1;
	ulong intindex;
	uint strindexlen;
	HashPosition hpos;
	char * strindex;
	char * infilename;	int infilename_len;
	char * outfilename;	int outfilename_len;
	char * extracertsfilename = NULL; int extracertsfilename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssZZa!|ls",
				&infilename, &infilename_len, &outfilename, &outfilename_len,
				&zcert, &zprivkey, &zheaders, &flags, &extracertsfilename,
				&extracertsfilename_len) == FAILURE) {
		return;
	}
	
	RETVAL_FALSE;

	if (extracertsfilename) {
		others = load_all_certs_from_file(extracertsfilename);
		if (others == NULL) { 
			goto clean_exit;
		}
	}

	privkey = php_openssl_evp_from_zval(zprivkey, 0, "", 0, &keyresource TSRMLS_CC);
	if (privkey == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "error getting private key");
		goto clean_exit;
	}

	cert = php_openssl_x509_from_zval(zcert, 0, &certresource TSRMLS_CC);
	if (cert == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "error getting cert");
		goto clean_exit;
	}

	if (php_openssl_safe_mode_chk(infilename TSRMLS_CC) || php_openssl_safe_mode_chk(outfilename TSRMLS_CC)) {
		goto clean_exit;
	}

	infile = BIO_new_file(infilename, "r");
	if (infile == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "error opening input file %s!", infilename);
		goto clean_exit;
	}

	outfile = BIO_new_file(outfilename, "w");
	if (outfile == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "error opening output file %s!", outfilename);
		goto clean_exit;
	}

	p7 = PKCS7_sign(cert, privkey, others, infile, flags);
	if (p7 == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "error creating PKCS7 structure!");
		goto clean_exit;
	}

	(void)BIO_reset(infile);

	/* tack on extra headers */
	if (zheaders) {
		zend_hash_internal_pointer_reset_ex(HASH_OF(zheaders), &hpos);
		while(zend_hash_get_current_data_ex(HASH_OF(zheaders), (void**)&hval, &hpos) == SUCCESS) {
			strindex = NULL;
			zend_hash_get_current_key_ex(HASH_OF(zheaders), &strindex, &strindexlen, &intindex, 0, &hpos);

			convert_to_string_ex(hval);

			if (strindex) {
				BIO_printf(outfile, "%s: %s\n", strindex, Z_STRVAL_PP(hval));
			} else {
				BIO_printf(outfile, "%s\n", Z_STRVAL_PP(hval));
			}
			zend_hash_move_forward_ex(HASH_OF(zheaders), &hpos);
		}
	}
	/* write the signed data */
	SMIME_write_PKCS7(outfile, p7, infile, flags);

	RETVAL_TRUE;

clean_exit:
	PKCS7_free(p7);
	BIO_free(infile);
	BIO_free(outfile);
	if (others) {
		sk_X509_pop_free(others, X509_free);
	}
	if (privkey && keyresource == -1) {
		EVP_PKEY_free(privkey);
	}
	if (cert && certresource == -1) {
		X509_free(cert);
	}
}