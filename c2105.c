static void php_do_chgrp(INTERNAL_FUNCTION_PARAMETERS, int do_lchgrp) /* {{{ */
{
	char *filename;
	int filename_len;
	zval *group;
	gid_t gid;
	int ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz/", &filename, &filename_len, &group) == FAILURE) {
		RETURN_FALSE;
	}

	if (Z_TYPE_P(group) == IS_LONG) {
		gid = (gid_t)Z_LVAL_P(group);
	} else if (Z_TYPE_P(group) == IS_STRING) {
#if defined(ZTS) && defined(HAVE_GETGRNAM_R) && defined(_SC_GETGR_R_SIZE_MAX)
		struct group gr;
		struct group *retgrptr;
		long grbuflen = sysconf(_SC_GETGR_R_SIZE_MAX);
		char *grbuf;

		if (grbuflen < 1) {
			RETURN_FALSE;
		}

		grbuf = emalloc(grbuflen);
		if (getgrnam_r(Z_STRVAL_P(group), &gr, grbuf, grbuflen, &retgrptr) != 0 || retgrptr == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to find gid for %s", Z_STRVAL_P(group));
			efree(grbuf);
			RETURN_FALSE;
		}
		efree(grbuf);
		gid = gr.gr_gid;
#else
		struct group *gr = getgrnam(Z_STRVAL_P(group));

		if (!gr) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to find gid for %s", Z_STRVAL_P(group));
			RETURN_FALSE;
		}
		gid = gr->gr_gid;
#endif
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "parameter 2 should be string or integer, %s given", zend_zval_type_name(group));
		RETURN_FALSE;
	}

	if (PG(safe_mode) &&(!php_checkuid(filename, NULL, CHECKUID_ALLOW_FILE_NOT_EXISTS))) {
		RETURN_FALSE;
	}

	/* Check the basedir */
	if (php_check_open_basedir(filename TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if (do_lchgrp) {
#if HAVE_LCHOWN
		ret = VCWD_LCHOWN(filename, -1, gid);
#endif
	} else {
		ret = VCWD_CHOWN(filename, -1, gid);
	}
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}