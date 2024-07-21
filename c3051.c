PHP_FUNCTION(xsl_xsltprocessor_import_stylesheet)
{
	zval *id, *docp = NULL;
	xmlDoc *doc = NULL, *newdoc = NULL;
	xsltStylesheetPtr sheetp, oldsheetp;
	xsl_object *intern;
	int prevSubstValue, prevExtDtdValue, clone_docu = 0;
	xmlNode *nodep = NULL;
	zend_object_handlers *std_hnd;
	zval *cloneDocu, *member;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oo", &id, xsl_xsltprocessor_class_entry, &docp) == FAILURE) {
		RETURN_FALSE;
	}

	nodep = php_libxml_import_node(docp TSRMLS_CC);
	
	if (nodep) {
		doc = nodep->doc;
	}
	if (doc == NULL) {
		php_error(E_WARNING, "Invalid Document");
		RETURN_FALSE;
	}

	/* libxslt uses _private, so we must copy the imported 
	stylesheet document otherwise the node proxies will be a mess */
	newdoc = xmlCopyDoc(doc, 1);
	xmlNodeSetBase((xmlNodePtr) newdoc, (xmlChar *)doc->URL);
	prevSubstValue = xmlSubstituteEntitiesDefault(1);
	prevExtDtdValue = xmlLoadExtDtdDefaultValue;
	xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;

	sheetp = xsltParseStylesheetDoc(newdoc);
	xmlSubstituteEntitiesDefault(prevSubstValue);
	xmlLoadExtDtdDefaultValue = prevExtDtdValue;

	if (!sheetp) {
		xmlFreeDoc(newdoc);
		RETURN_FALSE;
	}

	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC); 

	std_hnd = zend_get_std_object_handlers();
	MAKE_STD_ZVAL(member);
	ZVAL_STRING(member, "cloneDocument", 0);
	cloneDocu = std_hnd->read_property(id, member, BP_VAR_IS, NULL TSRMLS_CC);
	if (Z_TYPE_P(cloneDocu) != IS_NULL) {
		convert_to_long(cloneDocu);
		clone_docu = Z_LVAL_P(cloneDocu);
	}
	efree(member);
	if (clone_docu == 0) {
		/* check if the stylesheet is using xsl:key, if yes, we have to clone the document _always_ before a transformation */
		nodep = xmlDocGetRootElement(sheetp->doc);
		if (nodep && (nodep = nodep->children)) {
			while (nodep) {
				if (nodep->type == XML_ELEMENT_NODE && xmlStrEqual(nodep->name, "key") && xmlStrEqual(nodep->ns->href, XSLT_NAMESPACE)) {
					intern->hasKeys = 1;
					break;
				}
				nodep = nodep->next;
			}
		}
	} else {
		intern->hasKeys = clone_docu;
	}

	if ((oldsheetp = (xsltStylesheetPtr)intern->ptr)) { 
		/* free wrapper */
		if (((xsltStylesheetPtr) intern->ptr)->_private != NULL) {
			((xsltStylesheetPtr) intern->ptr)->_private = NULL;   
		}
		xsltFreeStylesheet((xsltStylesheetPtr) intern->ptr);
		intern->ptr = NULL;
	}

	php_xsl_set_object(id, sheetp TSRMLS_CC);
	RETVAL_TRUE;
}