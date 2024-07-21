PHP_FUNCTION(file_put_contents)
{
	php_stream *stream;
	char *filename;
	int filename_len;
	zval *data;
	int numbytes = 0;
	long flags = 0;
	zval *zcontext = NULL;
	php_stream_context *context = NULL;
	php_stream *srcstream = NULL;
	char mode[3] = "wb";

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz/|lr!", &filename, &filename_len, &data, &flags, &zcontext) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(data) == IS_RESOURCE) {
		php_stream_from_zval(srcstream, &data);
	}

	context = php_stream_context_from_zval(zcontext, flags & PHP_FILE_NO_DEFAULT_CONTEXT);

	if (flags & PHP_FILE_APPEND) {
		mode[0] = 'a';
	} else if (flags & LOCK_EX) {
		/* check to make sure we are dealing with a regular file */
		if (php_memnstr(filename, "://", sizeof("://") - 1, filename + filename_len)) {
			if (strncasecmp(filename, "file://", sizeof("file://") - 1)) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Exclusive locks may only be set for regular files");
				RETURN_FALSE;
			}
		}
		mode[0] = 'c';
	}
	mode[2] = '\0';

	stream = php_stream_open_wrapper_ex(filename, mode, ((flags & PHP_FILE_USE_INCLUDE_PATH) ? USE_PATH : 0) | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL, context);
	if (stream == NULL) {
		RETURN_FALSE;
	}

	if (flags & LOCK_EX && (!php_stream_supports_lock(stream) || php_stream_lock(stream, LOCK_EX))) {
		php_stream_close(stream);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Exclusive locks are not supported for this stream");
		RETURN_FALSE;
	}

	if (mode[0] == 'c') {
		php_stream_truncate_set_size(stream, 0);
	}

	switch (Z_TYPE_P(data)) {
		case IS_RESOURCE: {
			size_t len;
			if (php_stream_copy_to_stream_ex(srcstream, stream, PHP_STREAM_COPY_ALL, &len) != SUCCESS) {
				numbytes = -1;
			} else {
				numbytes = len;
			}
			break;
		}
		case IS_NULL:
		case IS_LONG:
		case IS_DOUBLE:
		case IS_BOOL:
		case IS_CONSTANT:
			convert_to_string_ex(&data);

		case IS_STRING:
			if (Z_STRLEN_P(data)) {
				numbytes = php_stream_write(stream, Z_STRVAL_P(data), Z_STRLEN_P(data));
				if (numbytes != Z_STRLEN_P(data)) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Only %d of %d bytes written, possibly out of free disk space", numbytes, Z_STRLEN_P(data));
					numbytes = -1;
				}
			}
			break;

		case IS_ARRAY:
			if (zend_hash_num_elements(Z_ARRVAL_P(data))) {
				int bytes_written;
				zval **tmp;
				HashPosition pos;

				zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(data), &pos);
				while (zend_hash_get_current_data_ex(Z_ARRVAL_P(data), (void **) &tmp, &pos) == SUCCESS) {
					if (Z_TYPE_PP(tmp) != IS_STRING) {
						SEPARATE_ZVAL(tmp);
						convert_to_string(*tmp);
					}
					if (Z_STRLEN_PP(tmp)) {
						numbytes += Z_STRLEN_PP(tmp);
						bytes_written = php_stream_write(stream, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
						if (bytes_written < 0 || bytes_written != Z_STRLEN_PP(tmp)) {
							if (bytes_written < 0) {
								php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to write %d bytes to %s", Z_STRLEN_PP(tmp), filename);
							} else {
								php_error_docref(NULL TSRMLS_CC, E_WARNING, "Only %d of %d bytes written, possibly out of free disk space", bytes_written, Z_STRLEN_PP(tmp));
							}
							numbytes = -1;
							break;
						}
					}
					zend_hash_move_forward_ex(Z_ARRVAL_P(data), &pos);
				}
			}
			break;

		case IS_OBJECT:
			if (Z_OBJ_HT_P(data) != NULL) {
				zval out;

				if (zend_std_cast_object_tostring(data, &out, IS_STRING TSRMLS_CC) == SUCCESS) {
					numbytes = php_stream_write(stream, Z_STRVAL(out), Z_STRLEN(out));
					if (numbytes != Z_STRLEN(out)) {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Only %d of %d bytes written, possibly out of free disk space", numbytes, Z_STRLEN(out));
						numbytes = -1;
					}
					zval_dtor(&out);
					break;
				}
			}
		default:
			numbytes = -1;
			break;
	}
	php_stream_close(stream);

	if (numbytes < 0) {
		RETURN_FALSE;
	}

	RETURN_LONG(numbytes);
}