static xmlDocPtr php_xsl_apply_stylesheet(zval *id, xsl_object *intern, xsltStylesheetPtr style, zval *docp TSRMLS_DC) /* {{{ */
{
	xmlDocPtr newdocp = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr node = NULL;
	xsltTransformContextPtr ctxt;
	php_libxml_node_object *object;
	char **params = NULL;
	int clone;
	zval *doXInclude, *member;
	zend_object_handlers *std_hnd;
	FILE *f;
	int secPrefsError = 0;
	int secPrefsValue, secPrefsIni;
	xsltSecurityPrefsPtr secPrefs = NULL;

	node = php_libxml_import_node(docp TSRMLS_CC);
	
	if (node) {
		doc = node->doc;
	}
	if (doc == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid Document");
		return NULL;
	}

	if (style == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No stylesheet associated to this object");
		return NULL;
	}
	
	if (intern->profiling) {
		if (php_check_open_basedir(intern->profiling TSRMLS_CC)) {
			f = NULL;
		} else {
			f = VCWD_FOPEN(intern->profiling, "w");
		}
	} else {
		f = NULL;
	}
	
	if (intern->parameter) {
		params = php_xsl_xslt_make_params(intern->parameter, 0 TSRMLS_CC);
	}

	intern->doc = emalloc(sizeof(php_libxml_node_object));
	memset(intern->doc, 0, sizeof(php_libxml_node_object));

	if (intern->hasKeys == 1) {
		doc = xmlCopyDoc(doc, 1);
	} else {
		object = (php_libxml_node_object *)zend_object_store_get_object(docp TSRMLS_CC);
		intern->doc->document = object->document;
	}

	php_libxml_increment_doc_ref(intern->doc, doc TSRMLS_CC);

	ctxt = xsltNewTransformContext(style, doc);
	ctxt->_private = (void *) intern;

	std_hnd = zend_get_std_object_handlers();

	MAKE_STD_ZVAL(member);
	ZVAL_STRING(member, "doXInclude", 0);
	doXInclude = std_hnd->read_property(id, member, BP_VAR_IS, NULL TSRMLS_CC);
	if (Z_TYPE_P(doXInclude) != IS_NULL) {
		convert_to_long(doXInclude);
		ctxt->xinclude = Z_LVAL_P(doXInclude);
	}
	efree(member);

	secPrefsValue = intern->securityPrefs;
	
	/* This whole if block can be removed, when we remove the xsl.security_prefs php.ini option in PHP 6+ */
	secPrefsIni= INI_INT("xsl.security_prefs");
	/* if secPrefsIni has the same value as secPrefsValue, all is fine */
	if (secPrefsIni != secPrefsValue) {
		if (secPrefsIni != XSL_SECPREF_DEFAULT) {
			/* if the ini value is not set to the default, throw an E_DEPRECATED warning */
			php_error_docref(NULL TSRMLS_CC, E_DEPRECATED, "The xsl.security_prefs php.ini option is deprecated; use XsltProcessor->setSecurityPrefs() instead");
			if (intern->securityPrefsSet == 0) {
				/* if securityPrefs were not set through the setSecurityPrefs method, take the ini setting */
				secPrefsValue = secPrefsIni;
			} else {
				/* else throw a notice, that the ini setting was not used */
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "The xsl.security_prefs php.ini was not used, since the  XsltProcessor->setSecurityPrefs() method was used");
			}
		}
	}

	/* if securityPrefs is set to NONE, we don't have to do any checks, but otherwise... */
	if (secPrefsValue != XSL_SECPREF_NONE) {
		secPrefs = xsltNewSecurityPrefs(); 
		if (secPrefsValue & XSL_SECPREF_READ_FILE ) { 
			if (0 != xsltSetSecurityPrefs(secPrefs, XSLT_SECPREF_READ_FILE, xsltSecurityForbid)) { 
				secPrefsError = 1;
			}
		}
		if (secPrefsValue & XSL_SECPREF_WRITE_FILE ) { 
			if (0 != xsltSetSecurityPrefs(secPrefs, XSLT_SECPREF_WRITE_FILE, xsltSecurityForbid)) { 
				secPrefsError = 1;
			}
		}
		if (secPrefsValue & XSL_SECPREF_CREATE_DIRECTORY ) { 
			if (0 != xsltSetSecurityPrefs(secPrefs, XSLT_SECPREF_CREATE_DIRECTORY, xsltSecurityForbid)) { 
				secPrefsError = 1;
			}
		}
		if (secPrefsValue & XSL_SECPREF_READ_NETWORK) { 
			if (0 != xsltSetSecurityPrefs(secPrefs, XSLT_SECPREF_READ_NETWORK, xsltSecurityForbid)) { 
				secPrefsError = 1;
			}
		}
		if (secPrefsValue & XSL_SECPREF_WRITE_NETWORK) { 
			if (0 != xsltSetSecurityPrefs(secPrefs, XSLT_SECPREF_WRITE_NETWORK, xsltSecurityForbid)) { 
				secPrefsError = 1;
			}
		}
	
		if (0 != xsltSetCtxtSecurityPrefs(secPrefs, ctxt)) { 
			secPrefsError = 1;
		}
	}
	
	if (secPrefsError == 1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't set libxslt security properties, not doing transformation for security reasons");
	} else {
		newdocp = xsltApplyStylesheetUser(style, doc, (const char**) params,  NULL, f, ctxt);
	}
	if (f) {
		fclose(f);
	}
	
	xsltFreeTransformContext(ctxt);
	if (secPrefs) {
		xsltFreeSecurityPrefs(secPrefs);
	}

	if (intern->node_list != NULL) {
		zend_hash_destroy(intern->node_list);
		FREE_HASHTABLE(intern->node_list);	
		intern->node_list = NULL;
	}

	php_libxml_decrement_doc_ref(intern->doc TSRMLS_CC);
	efree(intern->doc);
	intern->doc = NULL;

	if (params) {
		clone = 0;
		while(params[clone]) {
			efree(params[clone++]);
		}
		efree(params);
	}

	return newdocp;

}