PHP_FUNCTION(dom_document_save)
{
	zval *id;
	xmlDoc *docp;
	int file_len = 0, bytes, format, saveempty = 0;
	dom_object *intern;
	dom_doc_propsptr doc_props;
	char *file;
	long options = 0;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l", &id, dom_document_class_entry, &file, &file_len, &options) == FAILURE) {
		return;
	}

	if (file_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Filename");
		RETURN_FALSE;
	}

	DOM_GET_OBJ(docp, id, xmlDocPtr, intern);

	/* encoding handled by property on doc */

	doc_props = dom_get_doc_props(intern->document);
	format = doc_props->formatoutput;
	if (options & LIBXML_SAVE_NOEMPTYTAG) {
		saveempty = xmlSaveNoEmptyTags;
		xmlSaveNoEmptyTags = 1;
	}
	bytes = xmlSaveFormatFileEnc(file, docp, NULL, format);
	if (options & LIBXML_SAVE_NOEMPTYTAG) {
		xmlSaveNoEmptyTags = saveempty;
	}
	if (bytes == -1) {
		RETURN_FALSE;
	}
	RETURN_LONG(bytes);
}