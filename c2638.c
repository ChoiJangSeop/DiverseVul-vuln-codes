static void php_hash_do_hash_hmac(INTERNAL_FUNCTION_PARAMETERS, int isfilename, zend_bool raw_output_default) /* {{{ */
{
	char *algo, *data, *digest, *key, *K;
	int algo_len, data_len, key_len;
	zend_bool raw_output = raw_output_default;
	const php_hash_ops *ops;
	void *context;
	php_stream *stream = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|b", &algo, &algo_len, &data, &data_len, 
																  &key, &key_len, &raw_output) == FAILURE) {
		return;
	}

	ops = php_hash_fetch_ops(algo, algo_len);
	if (!ops) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown hashing algorithm: %s", algo);
		RETURN_FALSE;
	}
	if (isfilename) {
		stream = php_stream_open_wrapper_ex(data, "rb", REPORT_ERRORS, NULL, DEFAULT_CONTEXT);
		if (!stream) {
			/* Stream will report errors opening file */
			RETURN_FALSE;
		}
	}

	context = emalloc(ops->context_size);

	K = emalloc(ops->block_size);
	digest = emalloc(ops->digest_size + 1);

	php_hash_hmac_prep_key((unsigned char *) K, ops, context, (unsigned char *) key, key_len);		

	if (isfilename) {
		char buf[1024];
		int n;
		ops->hash_init(context);
		ops->hash_update(context, (unsigned char *) K, ops->block_size);
		while ((n = php_stream_read(stream, buf, sizeof(buf))) > 0) {
			ops->hash_update(context, (unsigned char *) buf, n);
		}
		php_stream_close(stream);
		ops->hash_final((unsigned char *) digest, context);
	} else {
		php_hash_hmac_round((unsigned char *) digest, ops, context, (unsigned char *) K, (unsigned char *) data, data_len);
	}

	php_hash_string_xor_char((unsigned char *) K, (unsigned char *) K, 0x6A, ops->block_size);

	php_hash_hmac_round((unsigned char *) digest, ops, context, (unsigned char *) K, (unsigned char *) digest, ops->digest_size);

	/* Zero the key */
	memset(K, 0, ops->block_size);
	efree(K);
	efree(context);

	if (raw_output) {
		digest[ops->digest_size] = 0;
		RETURN_STRINGL(digest, ops->digest_size, 0);
	} else {
		char *hex_digest = safe_emalloc(ops->digest_size, 2, 1);

		php_hash_bin2hex(hex_digest, (unsigned char *) digest, ops->digest_size);
		hex_digest[2 * ops->digest_size] = 0;
		efree(digest);
		RETURN_STRINGL(hex_digest, 2 * ops->digest_size, 0);
	}
}