static int php_zip_extract_file(struct zip * za, char *dest, char *file, int file_len TSRMLS_DC)
{
	php_stream_statbuf ssb;
	struct zip_file *zf;
	struct zip_stat sb;
	char b[8192];
	int n, len, ret;
	php_stream *stream;
	char *fullpath;
	char *file_dirname_fullpath;
	char file_dirname[MAXPATHLEN];
	size_t dir_len;
	char *file_basename;
	size_t file_basename_len;
	int is_dir_only = 0;
	char *path_cleaned;
	size_t path_cleaned_len;
	cwd_state new_state;

	new_state.cwd = (char*)malloc(1);
	new_state.cwd[0] = '\0';
	new_state.cwd_length = 0;

	/* Clean/normlize the path and then transform any path (absolute or relative)
		 to a path relative to cwd (../../mydir/foo.txt > mydir/foo.txt)
	 */
	virtual_file_ex(&new_state, file, NULL, CWD_EXPAND TSRMLS_CC);
	path_cleaned =  php_zip_make_relative_path(new_state.cwd, new_state.cwd_length);
	if(!path_cleaned) {
		return 0;
	}
	path_cleaned_len = strlen(path_cleaned);

	if (path_cleaned_len >= MAXPATHLEN || zip_stat(za, file, 0, &sb) != 0) {
		return 0;
	}

	/* it is a directory only, see #40228 */
	if (path_cleaned_len > 1 && IS_SLASH(path_cleaned[path_cleaned_len - 1])) {
		len = spprintf(&file_dirname_fullpath, 0, "%s/%s", dest, file);
		is_dir_only = 1;
	} else {
		memcpy(file_dirname, path_cleaned, path_cleaned_len);
		dir_len = php_dirname(file_dirname, path_cleaned_len);

		if (dir_len <= 0 || (dir_len == 1 && file_dirname[0] == '.')) {
			len = spprintf(&file_dirname_fullpath, 0, "%s", dest);
		} else {
			len = spprintf(&file_dirname_fullpath, 0, "%s/%s", dest, file_dirname);
		}

		php_basename(path_cleaned, path_cleaned_len, NULL, 0, &file_basename, (size_t *)&file_basename_len TSRMLS_CC);

		if (ZIP_OPENBASEDIR_CHECKPATH(file_dirname_fullpath)) {
			efree(file_dirname_fullpath);
			efree(file_basename);
			free(new_state.cwd);
			return 0;
		}
	}

	/* let see if the path already exists */
	if (php_stream_stat_path_ex(file_dirname_fullpath, PHP_STREAM_URL_STAT_QUIET, &ssb, NULL) < 0) {

#if defined(PHP_WIN32) && (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION == 1)
		char *e;
		e = file_dirname_fullpath;
		while (*e) {
			   if (*e == '/') {
					   *e = DEFAULT_SLASH;
			   }
			   e++;
		}
#endif

		ret = php_stream_mkdir(file_dirname_fullpath, 0777,  PHP_STREAM_MKDIR_RECURSIVE|REPORT_ERRORS, NULL);
		if (!ret) {
			efree(file_dirname_fullpath);
			if (!is_dir_only) {
				efree(file_basename);
				free(new_state.cwd);
			}
			return 0;
		}
	}

	/* it is a standalone directory, job done */
	if (is_dir_only) {
		efree(file_dirname_fullpath);
		free(new_state.cwd);
		return 1;
	}

	len = spprintf(&fullpath, 0, "%s/%s", file_dirname_fullpath, file_basename);
	if (!len) {
		efree(file_dirname_fullpath);
		efree(file_basename);
		free(new_state.cwd);
		return 0;
	} else if (len > MAXPATHLEN) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Full extraction path exceed MAXPATHLEN (%i)", MAXPATHLEN);
		efree(file_dirname_fullpath);
		efree(file_basename);
		free(new_state.cwd);
		return 0;
	}

	/* check again the full path, not sure if it
	 * is required, does a file can have a different
	 * safemode status as its parent folder?
	 */
	if (ZIP_OPENBASEDIR_CHECKPATH(fullpath)) {
		efree(fullpath);
		efree(file_dirname_fullpath);
		efree(file_basename);
		free(new_state.cwd);
		return 0;
	}

#if PHP_API_VERSION < 20100412
	stream = php_stream_open_wrapper(fullpath, "w+b", REPORT_ERRORS|ENFORCE_SAFE_MODE, NULL);
#else
	stream = php_stream_open_wrapper(fullpath, "w+b", REPORT_ERRORS, NULL);
#endif

	if (stream == NULL) {
		n = -1;
		goto done;
	}

	zf = zip_fopen(za, file, 0);
	if (zf == NULL) {
		n = -1;
		php_stream_close(stream);
		goto done;
	}

	n = 0;

	while ((n=zip_fread(zf, b, sizeof(b))) > 0) {
		php_stream_write(stream, b, n);
	}

	php_stream_close(stream);
	n = zip_fclose(zf);

done:
	efree(fullpath);
	efree(file_basename);
	efree(file_dirname_fullpath);
	free(new_state.cwd);

	if (n<0) {
		return 0;
	} else {
		return 1;
	}
}