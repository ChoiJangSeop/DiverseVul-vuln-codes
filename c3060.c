static int php_zip_add_file(struct zip *za, const char *filename, size_t filename_len, 
	char *entry_name, size_t entry_name_len, long offset_start, long offset_len TSRMLS_DC) /* {{{ */
{
	struct zip_source *zs;
	int cur_idx;
	char resolved_path[MAXPATHLEN];
	zval exists_flag;


	if (ZIP_OPENBASEDIR_CHECKPATH(filename)) {
		return -1;
	}

	if (!expand_filepath(filename, resolved_path TSRMLS_CC)) {
		return -1;
	}

	php_stat(resolved_path, strlen(resolved_path), FS_EXISTS, &exists_flag TSRMLS_CC);
	if (!Z_BVAL(exists_flag)) {
		return -1;
	}

	zs = zip_source_file(za, resolved_path, offset_start, offset_len);
	if (!zs) {
		return -1;
	}

	cur_idx = zip_name_locate(za, (const char *)entry_name, 0);
	/* TODO: fix  _zip_replace */
	if (cur_idx<0) {
		/* reset the error */
		if (za->error.str) {
			_zip_error_fini(&za->error);
		}
		_zip_error_init(&za->error);
	} else {
		if (zip_delete(za, cur_idx) == -1) {
			zip_source_free(zs);
			return -1;
		}
	}

	if (zip_add(za, entry_name, zs) == -1) {
		return -1;
	} else {
		return 1;
	}
}