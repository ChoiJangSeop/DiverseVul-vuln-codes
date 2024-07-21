static xmlDocPtr dom_document_parser(zval *id, int mode, char *source, int source_len, int options TSRMLS_DC) /* {{{ */
{
    xmlDocPtr ret;
    xmlParserCtxtPtr ctxt = NULL;
	dom_doc_propsptr doc_props;
	dom_object *intern;
	php_libxml_ref_obj *document = NULL;
	int validate, recover, resolve_externals, keep_blanks, substitute_ent;
	int resolved_path_len;
	int old_error_reporting = 0;
	char *directory=NULL, resolved_path[MAXPATHLEN];

	if (id != NULL) {
		intern = (dom_object *)zend_object_store_get_object(id TSRMLS_CC);
		document = intern->document;
	}

	doc_props = dom_get_doc_props(document);
	validate = doc_props->validateonparse;
	resolve_externals = doc_props->resolveexternals;
	keep_blanks = doc_props->preservewhitespace;
	substitute_ent = doc_props->substituteentities;
	recover = doc_props->recover;

	if (document == NULL) {
		efree(doc_props);
	}

	xmlInitParser();

	if (mode == DOM_LOAD_FILE) {
		char *file_dest = _dom_get_valid_file_path(source, resolved_path, MAXPATHLEN  TSRMLS_CC);
		if (file_dest) {
			ctxt = xmlCreateFileParserCtxt(file_dest);
		}
		
	} else {
		ctxt = xmlCreateMemoryParserCtxt(source, source_len);
	}

	if (ctxt == NULL) {
		return(NULL);
	}

	/* If loading from memory, we need to set the base directory for the document */
	if (mode != DOM_LOAD_FILE) {
#if HAVE_GETCWD
		directory = VCWD_GETCWD(resolved_path, MAXPATHLEN);
#elif HAVE_GETWD
		directory = VCWD_GETWD(resolved_path);
#endif
		if (directory) {
			if(ctxt->directory != NULL) {
				xmlFree((char *) ctxt->directory);
			}
			resolved_path_len = strlen(resolved_path);
			if (resolved_path[resolved_path_len - 1] != DEFAULT_SLASH) {
				resolved_path[resolved_path_len] = DEFAULT_SLASH;
				resolved_path[++resolved_path_len] = '\0';
			}
			ctxt->directory = (char *) xmlCanonicPath((const xmlChar *) resolved_path);
		}
	}

	ctxt->vctxt.error = php_libxml_ctx_error;
	ctxt->vctxt.warning = php_libxml_ctx_warning;

	if (ctxt->sax != NULL) {
		ctxt->sax->error = php_libxml_ctx_error;
		ctxt->sax->warning = php_libxml_ctx_warning;
	}

	if (validate && ! (options & XML_PARSE_DTDVALID)) {
		options |= XML_PARSE_DTDVALID;
	}
	if (resolve_externals && ! (options & XML_PARSE_DTDATTR)) {
		options |= XML_PARSE_DTDATTR;
	}
	if (substitute_ent && ! (options & XML_PARSE_NOENT)) {
		options |= XML_PARSE_NOENT;
	}
	if (keep_blanks == 0 && ! (options & XML_PARSE_NOBLANKS)) {
		options |= XML_PARSE_NOBLANKS;
	}

	xmlCtxtUseOptions(ctxt, options);

	ctxt->recovery = recover;
	if (recover) {
		old_error_reporting = EG(error_reporting);
		EG(error_reporting) = old_error_reporting | E_WARNING;
	}

	xmlParseDocument(ctxt);

	if (ctxt->wellFormed || recover) {
		ret = ctxt->myDoc;
		if (ctxt->recovery) {
			EG(error_reporting) = old_error_reporting;
		}
		/* If loading from memory, set the base reference uri for the document */
		if (ret && ret->URL == NULL && ctxt->directory != NULL) {
			ret->URL = xmlStrdup(ctxt->directory);
		}
	} else {
		ret = NULL;
		xmlFreeDoc(ctxt->myDoc);
		ctxt->myDoc = NULL;
	}

	xmlFreeParserCtxt(ctxt);

	return(ret);
}