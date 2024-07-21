PHP_METHOD(domimplementation, createDocumentType)
{
	xmlDtd *doctype;
	int ret;
	size_t name_len = 0, publicid_len = 0, systemid_len = 0;
	char *name = NULL, *publicid = NULL, *systemid = NULL;
	xmlChar *pch1 = NULL, *pch2 = NULL, *localname = NULL;
	xmlURIPtr uri;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|sss", &name, &name_len, &publicid, &publicid_len, &systemid, &systemid_len) == FAILURE) {
		return;
	}

	if (name_len == 0) {
		php_error_docref(NULL, E_WARNING, "qualifiedName is required");
		RETURN_FALSE;
	}

	if (publicid_len > 0) {
		pch1 = (xmlChar *) publicid;
	}
	if (systemid_len > 0) {
		pch2 = (xmlChar *) systemid;
	}

	uri = xmlParseURI(name);
	if (uri != NULL && uri->opaque != NULL) {
		localname = xmlStrdup((xmlChar *) uri->opaque);
		if (xmlStrchr(localname, (xmlChar) ':') != NULL) {
			php_dom_throw_error(NAMESPACE_ERR, 1);
			xmlFreeURI(uri);
			xmlFree(localname);
			RETURN_FALSE;
		}
	} else {
		localname = xmlStrdup((xmlChar *) name);
	}

	/* TODO: Test that localname has no invalid chars
	php_dom_throw_error(INVALID_CHARACTER_ERR,);
	*/

	if (uri) {
		xmlFreeURI(uri);
	}

	doctype = xmlCreateIntSubset(NULL, localname, pch1, pch2);
	xmlFree(localname);

	if (doctype == NULL) {
		php_error_docref(NULL, E_WARNING, "Unable to create DocumentType");
		RETURN_FALSE;
	}

	DOM_RET_OBJ((xmlNodePtr) doctype, &ret, NULL);
}