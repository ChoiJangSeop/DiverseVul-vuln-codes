static void php_do_chown(INTERNAL_FUNCTION_PARAMETERS, int do_lchown) /* {{{ */
{
	char *filename;
	int filename_len;
	zval *user;
	uid_t uid;
	int ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz/", &filename, &filename_len, &user) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(user) == IS_LONG) {
		uid = (uid_t)Z_LVAL_P(user);
	} else if (Z_TYPE_P(user) == IS_STRING) {
#if defined(ZTS) && defined(_SC_GETPW_R_SIZE_MAX) && defined(HAVE_GETPWNAM_R)
		struct passwd pw;
		struct passwd *retpwptr = NULL;
		long pwbuflen = sysconf(_SC_GETPW_R_SIZE_MAX);
		char *pwbuf;

		if (pwbuflen < 1) {
			RETURN_FALSE;
		}

		pwbuf = emalloc(pwbuflen);
		if (getpwnam_r(Z_STRVAL_P(user), &pw, pwbuf, pwbuflen, &retpwptr) != 0 || retpwptr == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to find uid for %s", Z_STRVAL_P(user));
			efree(pwbuf);
			RETURN_FALSE;
		}
		efree(pwbuf);
		uid = pw.pw_uid;
#else
		struct passwd *pw = getpwnam(Z_STRVAL_P(user));

		if (!pw) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to find uid for %s", Z_STRVAL_P(user));
			RETURN_FALSE;
		}
		uid = pw->pw_uid;
#endif
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "parameter 2 should be string or integer, %s given", zend_zval_type_name(user));
		RETURN_FALSE;
	}

	if (PG(safe_mode) && (!php_checkuid(filename, NULL, CHECKUID_ALLOW_FILE_NOT_EXISTS))) {
		RETURN_FALSE;
	}

	/* Check the basedir */
	if (php_check_open_basedir(filename TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if (do_lchown) {
#if HAVE_LCHOWN
		ret = VCWD_LCHOWN(filename, uid, -1);
#endif
	} else {
		ret = VCWD_CHOWN(filename, uid, -1);
	}
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}
}