PHP_FUNCTION(imagepsloadfont)
{
	char *file;
	int file_len, f_ind, *font;
#ifdef PHP_WIN32
	struct stat st;
#endif

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &file, &file_len) == FAILURE) {
		return;
	}

#ifdef PHP_WIN32
	if (VCWD_STAT(file, &st) < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Font file not found (%s)", file);
		RETURN_FALSE;
	}
#endif

	f_ind = T1_AddFont(file);

	if (f_ind < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "T1Lib Error (%i): %s", f_ind, T1_StrError(f_ind));
		RETURN_FALSE;
	}

	if (T1_LoadFont(f_ind)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Couldn't load the font");
		RETURN_FALSE;
	}

	font = (int *) emalloc(sizeof(int));
	*font = f_ind;
	ZEND_REGISTER_RESOURCE(return_value, font, le_ps_font);
}