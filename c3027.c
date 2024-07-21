static PHP_FUNCTION(preg_grep)
{
	char				*regex;			/* Regular expression */
	int				 	 regex_len;
	zval				*input;			/* Input array */
	long				 flags = 0;		/* Match control flags */
	pcre_cache_entry	*pce;			/* Compiled regular expression */

	/* Get arguments and do error checking */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa|l", &regex, &regex_len,
							  &input, &flags) == FAILURE) {
		return;
	}
	
	/* Compile regex or get it from cache. */
	if ((pce = pcre_get_compiled_regex_cache(regex, regex_len TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}
	
	php_pcre_grep_impl(pce, input, return_value, flags TSRMLS_CC);
}