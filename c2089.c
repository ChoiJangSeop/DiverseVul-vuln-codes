PHP_FUNCTION(glob)
{
	int cwd_skip = 0;
#ifdef ZTS
	char cwd[MAXPATHLEN];
	char work_pattern[MAXPATHLEN];
	char *result;
#endif
	char *pattern = NULL;
	int pattern_len;
	long flags = 0;
	glob_t globbuf;
	int n;
	int ret;
	zend_bool basedir_limit = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &pattern, &pattern_len, &flags) == FAILURE) {
		return;
	}

	if (pattern_len >= MAXPATHLEN) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Pattern exceeds the maximum allowed length of %d characters", MAXPATHLEN);
		RETURN_FALSE;
	}

	if ((GLOB_AVAILABLE_FLAGS & flags) != flags) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "At least one of the passed flags is invalid or not supported on this platform");
		RETURN_FALSE;
	}

#ifdef ZTS 
	if (!IS_ABSOLUTE_PATH(pattern, pattern_len)) {
		result = VCWD_GETCWD(cwd, MAXPATHLEN);	
		if (!result) {
			cwd[0] = '\0';
		}
#ifdef PHP_WIN32
		if (IS_SLASH(*pattern)) {
			cwd[2] = '\0';
		}
#endif
		cwd_skip = strlen(cwd)+1;

		snprintf(work_pattern, MAXPATHLEN, "%s%c%s", cwd, DEFAULT_SLASH, pattern);
		pattern = work_pattern;
	} 
#endif

	
	memset(&globbuf, 0, sizeof(glob_t));
	globbuf.gl_offs = 0;
	if (0 != (ret = glob(pattern, flags & GLOB_FLAGMASK, NULL, &globbuf))) {
#ifdef GLOB_NOMATCH
		if (GLOB_NOMATCH == ret) {
			/* Some glob implementation simply return no data if no matches
			   were found, others return the GLOB_NOMATCH error code.
			   We don't want to treat GLOB_NOMATCH as an error condition
			   so that PHP glob() behaves the same on both types of 
			   implementations and so that 'foreach (glob() as ...'
			   can be used for simple glob() calls without further error
			   checking.
			*/
			goto no_results;
		}
#endif
		RETURN_FALSE;
	}

	/* now catch the FreeBSD style of "no matches" */
	if (!globbuf.gl_pathc || !globbuf.gl_pathv) {
no_results:
		if (PG(safe_mode) || (PG(open_basedir) && *PG(open_basedir))) {
			struct stat s;

			if (0 != VCWD_STAT(pattern, &s) || S_IFDIR != (s.st_mode & S_IFMT)) {
				RETURN_FALSE;
			}
		}
		array_init(return_value);
		return;
	}

	array_init(return_value);
	for (n = 0; n < globbuf.gl_pathc; n++) {
		if (PG(safe_mode) || (PG(open_basedir) && *PG(open_basedir))) {
			if (PG(safe_mode) && (!php_checkuid_ex(globbuf.gl_pathv[n], NULL, CHECKUID_CHECK_FILE_AND_DIR, CHECKUID_NO_ERRORS))) {
				basedir_limit = 1;
				continue;
			} else if (php_check_open_basedir_ex(globbuf.gl_pathv[n], 0 TSRMLS_CC)) {
				basedir_limit = 1;
				continue;
			}
		}
		/* we need to do this everytime since GLOB_ONLYDIR does not guarantee that
		 * all directories will be filtered. GNU libc documentation states the
		 * following: 
		 * If the information about the type of the file is easily available 
		 * non-directories will be rejected but no extra work will be done to 
		 * determine the information for each file. I.e., the caller must still be 
		 * able to filter directories out. 
		 */
		if (flags & GLOB_ONLYDIR) {
			struct stat s;

			if (0 != VCWD_STAT(globbuf.gl_pathv[n], &s)) {
				continue;
			}

			if (S_IFDIR != (s.st_mode & S_IFMT)) {
				continue;
			}
		}
		add_next_index_string(return_value, globbuf.gl_pathv[n]+cwd_skip, 1);
	}

	globfree(&globbuf);

	if (basedir_limit && !zend_hash_num_elements(Z_ARRVAL_P(return_value))) {
		zval_dtor(return_value);
		RETURN_FALSE;
	}
}