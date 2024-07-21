static void dom_load_html(INTERNAL_FUNCTION_PARAMETERS, int mode) /* {{{ */
{
	zval *id;
	xmlDoc *docp = NULL, *newdoc;
	dom_object *intern;
	dom_doc_propsptr doc_prop;
	char *source;
	int source_len, refcount, ret;
	long options = 0;
	htmlParserCtxtPtr ctxt;
	
	id = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &source, &source_len, &options) == FAILURE) {
		return;
	}

	if (!source_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty string supplied as input");
		RETURN_FALSE;
	}

	if (mode == DOM_LOAD_FILE) {
		ctxt = htmlCreateFileParserCtxt(source, NULL);
	} else {
		source_len = xmlStrlen(source);
		ctxt = htmlCreateMemoryParserCtxt(source, source_len);
	}

	if (!ctxt) {
		RETURN_FALSE;
	}

	if (options) {
		htmlCtxtUseOptions(ctxt, options);
	}

	ctxt->vctxt.error = php_libxml_ctx_error;
	ctxt->vctxt.warning = php_libxml_ctx_warning;
	if (ctxt->sax != NULL) {
		ctxt->sax->error = php_libxml_ctx_error;
		ctxt->sax->warning = php_libxml_ctx_warning;
	}
	htmlParseDocument(ctxt);
	newdoc = ctxt->myDoc;
	htmlFreeParserCtxt(ctxt);
	
	if (!newdoc)
		RETURN_FALSE;

	if (id != NULL && instanceof_function(Z_OBJCE_P(id), dom_document_class_entry TSRMLS_CC)) {
		intern = (dom_object *)zend_object_store_get_object(id TSRMLS_CC);
		if (intern != NULL) {
			docp = (xmlDocPtr) dom_object_get_node(intern);
			doc_prop = NULL;
			if (docp != NULL) {
				php_libxml_decrement_node_ptr((php_libxml_node_object *) intern TSRMLS_CC);
				doc_prop = intern->document->doc_props;
				intern->document->doc_props = NULL;
				refcount = php_libxml_decrement_doc_ref((php_libxml_node_object *)intern TSRMLS_CC);
				if (refcount != 0) {
					docp->_private = NULL;
				}
			}
			intern->document = NULL;
			if (php_libxml_increment_doc_ref((php_libxml_node_object *)intern, newdoc TSRMLS_CC) == -1) {
				RETURN_FALSE;
			}
			intern->document->doc_props = doc_prop;
		}

		php_libxml_increment_node_ptr((php_libxml_node_object *)intern, (xmlNodePtr)newdoc, (void *)intern TSRMLS_CC);

		RETURN_TRUE;
	} else {
		DOM_RET_OBJ((xmlNodePtr) newdoc, &ret, NULL);
	}
}