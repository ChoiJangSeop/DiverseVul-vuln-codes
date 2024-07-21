PHP_FUNCTION(openssl_pkcs7_encrypt)
{
	zval ** zrecipcerts, * zheaders = NULL;
	STACK_OF(X509) * recipcerts = NULL;
	BIO * infile = NULL, * outfile = NULL;
	long flags = 0;
	PKCS7 * p7 = NULL;
	HashPosition hpos;
	zval ** zcertval;
	X509 * cert;
	const EVP_CIPHER *cipher = NULL;
	long cipherid = PHP_OPENSSL_CIPHER_DEFAULT;
	uint strindexlen;
	ulong intindex;
	char * strindex;
	char * infilename = NULL;	int infilename_len;
	char * outfilename = NULL;	int outfilename_len;
	
	RETVAL_FALSE;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssZa!|ll", &infilename, &infilename_len,
				&outfilename, &outfilename_len, &zrecipcerts, &zheaders, &flags, &cipherid) == FAILURE)
		return;

	
	if (php_openssl_safe_mode_chk(infilename TSRMLS_CC) || php_openssl_safe_mode_chk(outfilename TSRMLS_CC)) {
		return;
	}

	infile = BIO_new_file(infilename, "r");
	if (infile == NULL) {
		goto clean_exit;
	}

	outfile = BIO_new_file(outfilename, "w");
	if (outfile == NULL) { 
		goto clean_exit;
	}

	recipcerts = sk_X509_new_null();

	/* get certs */
	if (Z_TYPE_PP(zrecipcerts) == IS_ARRAY) {
		zend_hash_internal_pointer_reset_ex(HASH_OF(*zrecipcerts), &hpos);
		while(zend_hash_get_current_data_ex(HASH_OF(*zrecipcerts), (void**)&zcertval, &hpos) == SUCCESS) {
			long certresource;

			cert = php_openssl_x509_from_zval(zcertval, 0, &certresource TSRMLS_CC);
			if (cert == NULL) {
				goto clean_exit;
			}

			if (certresource != -1) {
				/* we shouldn't free this particular cert, as it is a resource.
					make a copy and push that on the stack instead */
				cert = X509_dup(cert);
				if (cert == NULL) {
					goto clean_exit;
				}
			}
			sk_X509_push(recipcerts, cert);

			zend_hash_move_forward_ex(HASH_OF(*zrecipcerts), &hpos);
		}
	} else {
		/* a single certificate */
		long certresource;

		cert = php_openssl_x509_from_zval(zrecipcerts, 0, &certresource TSRMLS_CC);
		if (cert == NULL) {
			goto clean_exit;
		}

		if (certresource != -1) {
			/* we shouldn't free this particular cert, as it is a resource.
				make a copy and push that on the stack instead */
			cert = X509_dup(cert);
			if (cert == NULL) {
				goto clean_exit;
			}
		}
		sk_X509_push(recipcerts, cert);
	}

	/* sanity check the cipher */
	cipher = php_openssl_get_evp_cipher_from_algo(cipherid);
	if (cipher == NULL) {
		/* shouldn't happen */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to get cipher");
		goto clean_exit;
	}

	p7 = PKCS7_encrypt(recipcerts, infile, (EVP_CIPHER*)cipher, flags);

	if (p7 == NULL) {
		goto clean_exit;
	}

	/* tack on extra headers */
	if (zheaders) {
		zend_hash_internal_pointer_reset_ex(HASH_OF(zheaders), &hpos);
		while(zend_hash_get_current_data_ex(HASH_OF(zheaders), (void**)&zcertval, &hpos) == SUCCESS) {
			strindex = NULL;
			zend_hash_get_current_key_ex(HASH_OF(zheaders), &strindex, &strindexlen, &intindex, 0, &hpos);

			convert_to_string_ex(zcertval);

			if (strindex) {
				BIO_printf(outfile, "%s: %s\n", strindex, Z_STRVAL_PP(zcertval));
			} else {
				BIO_printf(outfile, "%s\n", Z_STRVAL_PP(zcertval));
			}

			zend_hash_move_forward_ex(HASH_OF(zheaders), &hpos);
		}
	}

	(void)BIO_reset(infile);

	/* write the encrypted data */
	SMIME_write_PKCS7(outfile, p7, infile, flags);

	RETVAL_TRUE;

clean_exit:
	PKCS7_free(p7);
	BIO_free(infile);
	BIO_free(outfile);
	if (recipcerts) {
		sk_X509_pop_free(recipcerts, X509_free);
	}
}