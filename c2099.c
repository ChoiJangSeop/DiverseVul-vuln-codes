static PHP_FUNCTION(bzopen)
{
	zval    **file;   /* The file to open */
	char     *mode;   /* The mode to open the stream with */
	int      mode_len;

	BZFILE   *bz;     /* The compressed file stream */
	php_stream *stream = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Zs", &file, &mode, &mode_len) == FAILURE) {
		return;
	}

	if (mode_len != 1 || (mode[0] != 'r' && mode[0] != 'w')) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid mode for bzopen(). Only 'w' and 'r' are supported.", mode);
		RETURN_FALSE;
	}

	/* If it's not a resource its a string containing the filename to open */
	if (Z_TYPE_PP(file) == IS_STRING) {
		convert_to_string_ex(file);

		if (Z_STRLEN_PP(file) == 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "filename cannot be empty");
			RETURN_FALSE;
		}

		stream = php_stream_bz2open(NULL,
									Z_STRVAL_PP(file), 
									mode, 
									ENFORCE_SAFE_MODE | REPORT_ERRORS, 
									NULL);
	} else if (Z_TYPE_PP(file) == IS_RESOURCE) {
		/* If it is a resource, than its a stream resource */
		int fd;
		int stream_mode_len;

		php_stream_from_zval(stream, file);
		stream_mode_len = strlen(stream->mode);
		
		if (stream_mode_len != 1 && !(stream_mode_len == 2 && memchr(stream->mode, 'b', 2))) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot use stream opened in mode '%s'", stream->mode);
			RETURN_FALSE;
		} else if (stream_mode_len == 1 && stream->mode[0] != 'r' && stream->mode[0] != 'w' && stream->mode[0] != 'a' && stream->mode[0] != 'x') {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot use stream opened in mode '%s'", stream->mode);
			RETURN_FALSE;
		}

		switch(mode[0]) {
			case 'r':
				/* only "r" and "rb" are supported */
				if (stream->mode[0] != mode[0] && !(stream_mode_len == 2 && stream->mode[1] != mode[0])) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot read from a stream opened in write only mode");
					RETURN_FALSE;
				}
				break;
			case 'w':
				/* support only "w"(b), "a"(b), "x"(b) */
				if (stream->mode[0] != mode[0] && !(stream_mode_len == 2 && stream->mode[1] != mode[0])
					&& stream->mode[0] != 'a' && !(stream_mode_len == 2 && stream->mode[1] != 'a')
					&& stream->mode[0] != 'x' && !(stream_mode_len == 2 && stream->mode[1] != 'x')) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot write to a stream opened in read only mode");
					RETURN_FALSE;
				}
				break;
			default:
				/* not reachable */
				break;
		}

		if (FAILURE == php_stream_cast(stream, PHP_STREAM_AS_FD, (void *) &fd, REPORT_ERRORS)) {
			RETURN_FALSE;
		}
		
		bz = BZ2_bzdopen(fd, mode);

		stream = php_stream_bz2open_from_BZFILE(bz, mode, stream);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "first parameter has to be string or file-resource");
		RETURN_FALSE;
	}

	if (stream) {
		php_stream_to_zval(stream, return_value);
	} else {
		RETURN_FALSE;
	}
}