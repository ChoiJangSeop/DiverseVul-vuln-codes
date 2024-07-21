PHPAPI char *php_pcre_replace(char *regex,   int regex_len,
							  char *subject, int subject_len,
							  zval *replace_val, int is_callable_replace,
							  int *result_len, int limit, int *replace_count TSRMLS_DC)
{
	pcre_cache_entry	*pce;			    /* Compiled regular expression */

	/* Compile regex or get it from cache. */
	if ((pce = pcre_get_compiled_regex_cache(regex, regex_len TSRMLS_CC)) == NULL) {
		return NULL;
	}

	return php_pcre_replace_impl(pce, subject, subject_len, replace_val, 
		is_callable_replace, result_len, limit, replace_count TSRMLS_CC);
}