static void _dom_document_schema_validate(INTERNAL_FUNCTION_PARAMETERS, int type) /* {{{ */
{
	zval *id;
	xmlDoc *docp;
	dom_object *intern;
	char *source = NULL, *valid_file = NULL;
	int source_len = 0;
	xmlSchemaParserCtxtPtr  parser;
	xmlSchemaPtr            sptr;
	xmlSchemaValidCtxtPtr   vptr;
	int                     is_valid;
	char resolved_path[MAXPATHLEN + 1];

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Op", &id, dom_document_class_entry, &source, &source_len) == FAILURE) {
		return;
	}

	if (source_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Schema source");
		RETURN_FALSE;
	}

	DOM_GET_OBJ(docp, id, xmlDocPtr, intern);

	switch (type) {
	case DOM_LOAD_FILE:
		valid_file = _dom_get_valid_file_path(source, resolved_path, MAXPATHLEN  TSRMLS_CC);
		if (!valid_file) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Schema file source");
			RETURN_FALSE;
		}
		parser = xmlSchemaNewParserCtxt(valid_file);
		break;
	case DOM_LOAD_STRING:
		parser = xmlSchemaNewMemParserCtxt(source, source_len);
		/* If loading from memory, we need to set the base directory for the document
		   but it is not apparent how to do that for schema's */
		break;
	default:
		return;
	}

	xmlSchemaSetParserErrors(parser,
		(xmlSchemaValidityErrorFunc) php_libxml_error_handler,
		(xmlSchemaValidityWarningFunc) php_libxml_error_handler,
		parser);
	sptr = xmlSchemaParse(parser);
	xmlSchemaFreeParserCtxt(parser);
	if (!sptr) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Schema");
		RETURN_FALSE;
	}

	docp = (xmlDocPtr) dom_object_get_node(intern);

	vptr = xmlSchemaNewValidCtxt(sptr);
	if (!vptr) {
		xmlSchemaFree(sptr);
		php_error(E_ERROR, "Invalid Schema Validation Context");
		RETURN_FALSE;
	}

	xmlSchemaSetValidErrors(vptr, php_libxml_error_handler, php_libxml_error_handler, vptr);
	is_valid = xmlSchemaValidateDoc(vptr, docp);
	xmlSchemaFree(sptr);
	xmlSchemaFreeValidCtxt(vptr);

	if (is_valid == 0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}