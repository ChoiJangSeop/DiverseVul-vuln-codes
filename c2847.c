PHP_FUNCTION(dom_document_save_html_file)
{
	zval *id;
	xmlDoc *docp;
	int file_len, bytes, format;
	dom_object *intern;
	dom_doc_propsptr doc_props;
	char *file;
	const char *encoding;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &id, dom_document_class_entry, &file, &file_len) == FAILURE) {
		return;
	}

	if (file_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Filename");
		RETURN_FALSE;
	}

	DOM_GET_OBJ(docp, id, xmlDocPtr, intern);


	encoding = (const char *) htmlGetMetaEncoding(docp);

	doc_props = dom_get_doc_props(intern->document);
	format = doc_props->formatoutput;
	bytes = htmlSaveFileFormat(file, docp, encoding, format);

	if (bytes == -1) {
		RETURN_FALSE;
	}
	RETURN_LONG(bytes);
}