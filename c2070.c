PHP_FUNCTION(touch)
{
	char *filename;
	int filename_len;
	long filetime = 0, fileatime = 0;
	int ret, argc = ZEND_NUM_ARGS();
	FILE *file;
	struct utimbuf newtimebuf;
	struct utimbuf *newtime = &newtimebuf;

	if (zend_parse_parameters(argc TSRMLS_CC, "s|ll", &filename, &filename_len, &filetime, &fileatime) == FAILURE) {
		return;
	}

	switch (argc) {
		case 1:
#ifdef HAVE_UTIME_NULL
			newtime = NULL;
#else
			newtime->modtime = newtime->actime = time(NULL);
#endif
			break;
		case 2:
			newtime->modtime = newtime->actime = filetime;
			break;
		case 3:
			newtime->modtime = filetime;
			newtime->actime = fileatime;
			break;
		default:
			/* Never reached */
			WRONG_PARAM_COUNT;
	}

	/* Safe-mode */
	if (PG(safe_mode) && (!php_checkuid(filename, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
		RETURN_FALSE;
	}

	/* Check the basedir */
	if (php_check_open_basedir(filename TSRMLS_CC)) {
		RETURN_FALSE;
	}

	/* create the file if it doesn't exist already */
	if (VCWD_ACCESS(filename, F_OK) != 0) {
		file = VCWD_FOPEN(filename, "w");
		if (file == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to create file %s because %s", filename, strerror(errno));
			RETURN_FALSE;
		}
		fclose(file);
	}

	ret = VCWD_UTIME(filename, newtime);
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Utime failed: %s", strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}