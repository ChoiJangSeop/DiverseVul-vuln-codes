PHP_FUNCTION(chmod)
{
	char *filename;
	int filename_len;
	long mode;
	int ret;
	mode_t imode;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &filename, &filename_len, &mode) == FAILURE) {
		return;
	}

	if (PG(safe_mode) &&(!php_checkuid(filename, NULL, CHECKUID_ALLOW_FILE_NOT_EXISTS))) {
		RETURN_FALSE;
	}

	/* Check the basedir */
	if (php_check_open_basedir(filename TSRMLS_CC)) {
		RETURN_FALSE;
	}

	imode = (mode_t) mode;
	/* In safe mode, do not allow to setuid files.
	 * Setuiding files could allow users to gain privileges
	 * that safe mode doesn't give them. */

	if (PG(safe_mode)) {
		php_stream_statbuf ssb;
		if (php_stream_stat_path_ex(filename, 0, &ssb, NULL)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "stat failed for %s", filename);
			RETURN_FALSE;
		}
		if ((imode & 04000) != 0 && (ssb.sb.st_mode & 04000) == 0) {
			imode ^= 04000;
		}
		if ((imode & 02000) != 0 && (ssb.sb.st_mode & 02000) == 0) {
			imode ^= 02000;
		}
		if ((imode & 01000) != 0 && (ssb.sb.st_mode & 01000) == 0) {
			imode ^= 01000;
		}
	}

	ret = VCWD_CHMOD(filename, imode);
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}