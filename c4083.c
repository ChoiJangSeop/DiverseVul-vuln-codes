SAPI_API char *sapi_getenv(char *name, size_t name_len TSRMLS_DC)
{
	if (sapi_module.getenv) { 
		char *value, *tmp = sapi_module.getenv(name, name_len TSRMLS_CC);
		if (tmp) {
			value = estrdup(tmp);
		} else {
			return NULL;
		}
		if (sapi_module.input_filter) {
			sapi_module.input_filter(PARSE_STRING, name, &value, strlen(value), NULL TSRMLS_CC);
		}
		return value;
	}
	return NULL;
}