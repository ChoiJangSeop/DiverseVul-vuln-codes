static char *php_xsl_xslt_string_to_xpathexpr(const char *str TSRMLS_DC)
{
	const xmlChar *string = (const xmlChar *)str;

	xmlChar *value;
	int str_len;
	
	str_len = xmlStrlen(string) + 3;
	
	if (xmlStrchr(string, '"')) {
		if (xmlStrchr(string, '\'')) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create XPath expression (string contains both quote and double-quotes)");
			return NULL;
		}
		value = (xmlChar*) safe_emalloc (str_len, sizeof(xmlChar), 0);
		snprintf(value, str_len, "'%s'", string);
	} else {
		value = (xmlChar*) safe_emalloc (str_len, sizeof(xmlChar), 0);
		snprintf(value, str_len, "\"%s\"", string);
	}
	return (char *) value;
}