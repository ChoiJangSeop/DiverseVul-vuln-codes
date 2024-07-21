static void php_do_pcre_match(INTERNAL_FUNCTION_PARAMETERS, int global) /* {{{ */
{
	/* parameters */
	char			 *regex;			/* Regular expression */
	char			 *subject;			/* String to match against */
	int				  regex_len;
	int				  subject_len;
	pcre_cache_entry *pce;				/* Compiled regular expression */
	zval			 *subpats = NULL;	/* Array for subpatterns */
	long			  flags = 0;		/* Match control flags */
	long			  start_offset = 0;	/* Where the new search starts */

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|zll", &regex, &regex_len,
							  &subject, &subject_len, &subpats, &flags, &start_offset) == FAILURE) {
		RETURN_FALSE;
	}
	
	/* Compile regex or get it from cache. */
	if ((pce = pcre_get_compiled_regex_cache(regex, regex_len TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	php_pcre_match_impl(pce, subject, subject_len, return_value, subpats, 
		global, ZEND_NUM_ARGS() >= 4, flags, start_offset TSRMLS_CC);
}