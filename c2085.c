PHP_FUNCTION(file)
{
	char *filename;
	int filename_len;
	char *slashed, *target_buf=NULL, *p, *s, *e;
	register int i = 0;
	int target_len, len;
	char eol_marker = '\n';
	long flags = 0;
	zend_bool use_include_path;
	zend_bool include_new_line;
	zend_bool skip_blank_lines;
	php_stream *stream;
	zval *zcontext = NULL;
	php_stream_context *context = NULL;

	/* Parse arguments */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lr!", &filename, &filename_len, &flags, &zcontext) == FAILURE) {
		return;
	}
	if (flags < 0 || flags > (PHP_FILE_USE_INCLUDE_PATH | PHP_FILE_IGNORE_NEW_LINES | PHP_FILE_SKIP_EMPTY_LINES | PHP_FILE_NO_DEFAULT_CONTEXT)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%ld' flag is not supported", flags);
		RETURN_FALSE;
	}

	use_include_path = flags & PHP_FILE_USE_INCLUDE_PATH;
	include_new_line = !(flags & PHP_FILE_IGNORE_NEW_LINES);
	skip_blank_lines = flags & PHP_FILE_SKIP_EMPTY_LINES;

	context = php_stream_context_from_zval(zcontext, flags & PHP_FILE_NO_DEFAULT_CONTEXT);

	stream = php_stream_open_wrapper_ex(filename, "rb", (use_include_path ? USE_PATH : 0) | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL, context);
	if (!stream) {
		RETURN_FALSE;
	}

	/* Initialize return array */
	array_init(return_value);

	if ((target_len = php_stream_copy_to_mem(stream, &target_buf, PHP_STREAM_COPY_ALL, 0))) {
		s = target_buf;
		e = target_buf + target_len;

		if (!(p = php_stream_locate_eol(stream, target_buf, target_len TSRMLS_CC))) {
			p = e;
			goto parse_eol;
		}

		if (stream->flags & PHP_STREAM_FLAG_EOL_MAC) {
			eol_marker = '\r';
		}

		/* for performance reasons the code is duplicated, so that the if (include_new_line)
		 * will not need to be done for every single line in the file. */
		if (include_new_line) {
			do {
				p++;
parse_eol:
				if (PG(magic_quotes_runtime)) {
					/* s is in target_buf which is freed at the end of the function */
					slashed = php_addslashes(s, (p-s), &len, 0 TSRMLS_CC);
					add_index_stringl(return_value, i++, slashed, len, 0);
				} else {
					add_index_stringl(return_value, i++, estrndup(s, p-s), p-s, 0);
				}
				s = p;
			} while ((p = memchr(p, eol_marker, (e-p))));
		} else {
			do {
				int windows_eol = 0;
				if (p != target_buf && eol_marker == '\n' && *(p - 1) == '\r') {
					windows_eol++;
				}
				if (skip_blank_lines && !(p-s-windows_eol)) {
					s = ++p;
					continue;
				}
				if (PG(magic_quotes_runtime)) {
					/* s is in target_buf which is freed at the end of the function */
					slashed = php_addslashes(s, (p-s-windows_eol), &len, 0 TSRMLS_CC);
					add_index_stringl(return_value, i++, slashed, len, 0);
				} else {
					add_index_stringl(return_value, i++, estrndup(s, p-s-windows_eol), p-s-windows_eol, 0);
				}
				s = ++p;
			} while ((p = memchr(p, eol_marker, (e-p))));
		}

		/* handle any left overs of files without new lines */
		if (s != e) {
			p = e;
			goto parse_eol;
		}
	}

	if (target_buf) {
		efree(target_buf);
	}
	php_stream_close(stream);
}