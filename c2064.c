PHP_FUNCTION(scandir)
{
	char *dirn;
	int dirn_len;
	long flags = 0;
	char **namelist;
	int n, i;
	zval *zcontext = NULL;
	php_stream_context *context = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lr", &dirn, &dirn_len, &flags, &zcontext) == FAILURE) {
		return;
	}

	if (dirn_len < 1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Directory name cannot be empty");
		RETURN_FALSE;
	}

	if (zcontext) {
		context = php_stream_context_from_zval(zcontext, 0);
	}

	if (!flags) {
		n = php_stream_scandir(dirn, &namelist, context, (void *) php_stream_dirent_alphasort);
	} else {
		n = php_stream_scandir(dirn, &namelist, context, (void *) php_stream_dirent_alphasortr);
	}
	if (n < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "(errno %d): %s", errno, strerror(errno));
		RETURN_FALSE;
	}
	
	array_init(return_value);

	for (i = 0; i < n; i++) {
		add_next_index_string(return_value, namelist[i], 0);
	}

	if (n) {
		efree(namelist);
	}
}