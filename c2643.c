PHP_FUNCTION(hash_update_file)
{
	zval *zhash, *zcontext = NULL;
	php_hash_data *hash;
	php_stream_context *context;
	php_stream *stream;
	char *filename, buf[1024];
	int filename_len, n;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|r", &zhash, &filename, &filename_len, &zcontext) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(hash, php_hash_data*, &zhash, -1, PHP_HASH_RESNAME, php_hash_le_hash);
	context = php_stream_context_from_zval(zcontext, 0);

	stream = php_stream_open_wrapper_ex(filename, "rb", REPORT_ERRORS, NULL, context);
	if (!stream) {
		/* Stream will report errors opening file */
		RETURN_FALSE;
	}

	while ((n = php_stream_read(stream, buf, sizeof(buf))) > 0) {
		hash->ops->hash_update(hash->context, (unsigned char *) buf, n);
	}
	php_stream_close(stream);

	RETURN_TRUE;
}