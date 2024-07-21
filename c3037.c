PHPAPI pcre* pcre_get_compiled_regex(char *regex, pcre_extra **extra, int *preg_options TSRMLS_DC)
{
	pcre_cache_entry * pce = pcre_get_compiled_regex_cache(regex, strlen(regex) TSRMLS_CC);

	if (extra) {
		*extra = pce ? pce->extra : NULL;
	}
	if (preg_options) {
		*preg_options = pce ? pce->preg_options : 0;
	}
	
	return pce ? pce->re : NULL;
}