static void xsl_ext_function_php(xmlXPathParserContextPtr ctxt, int nargs, int type) /* {{{ */
{
	xsltTransformContextPtr tctxt;
	zval **args;
	zval *retval;
	int result, i, ret;
	int error = 0;
	zend_fcall_info fci;
	zval handler;
	xmlXPathObjectPtr obj;
	char *str;
	char *callable = NULL;
	xsl_object *intern;
	
	TSRMLS_FETCH();

	if (! zend_is_executing(TSRMLS_C)) {
		xsltGenericError(xsltGenericErrorContext,
		"xsltExtFunctionTest: Function called from outside of PHP\n");
		error = 1;
	} else {
		tctxt = xsltXPathGetTransformContext(ctxt);
		if (tctxt == NULL) {
			xsltGenericError(xsltGenericErrorContext,
			"xsltExtFunctionTest: failed to get the transformation context\n");
			error = 1;
		} else {
			intern = (xsl_object *) tctxt->_private;
			if (intern == NULL) {
				xsltGenericError(xsltGenericErrorContext,
				"xsltExtFunctionTest: failed to get the internal object\n");
				error = 1;
			}
			else if (intern->registerPhpFunctions == 0) {
				xsltGenericError(xsltGenericErrorContext,
				"xsltExtFunctionTest: PHP Object did not register PHP functions\n");
				error = 1;
			}
		}
	}
	
	if (error == 1) {
		for (i = nargs - 1; i >= 0; i--) {
			obj = valuePop(ctxt);
			xmlXPathFreeObject(obj);
		}
		return;
	}
		
	fci.param_count = nargs - 1;
	if (fci.param_count > 0) {
		fci.params = safe_emalloc(fci.param_count, sizeof(zval**), 0);
		args = safe_emalloc(fci.param_count, sizeof(zval *), 0);
	}
	/* Reverse order to pop values off ctxt stack */
	for (i = nargs - 2; i >= 0; i--) {
		obj = valuePop(ctxt);
		MAKE_STD_ZVAL(args[i]);
		switch (obj->type) {
			case XPATH_STRING:
				ZVAL_STRING(args[i],  obj->stringval, 1);
				break;
			case XPATH_BOOLEAN:
				ZVAL_BOOL(args[i],  obj->boolval);
				break;
			case XPATH_NUMBER:
				ZVAL_DOUBLE(args[i], obj->floatval);
				break;
			case XPATH_NODESET:
				if (type == 1) {
					str = xmlXPathCastToString(obj);
					ZVAL_STRING(args[i], str, 1);
					xmlFree(str);
				} else if (type == 2) {
					int j;
					dom_object *domintern = (dom_object *)intern->doc;
					array_init(args[i]);
					if (obj->nodesetval && obj->nodesetval->nodeNr > 0) {
						for (j = 0; j < obj->nodesetval->nodeNr; j++) {
							xmlNodePtr node = obj->nodesetval->nodeTab[j];
							zval *child;
							MAKE_STD_ZVAL(child);
							/* not sure, if we need this... it's copied from xpath.c */
							if (node->type == XML_NAMESPACE_DECL) {
								xmlNsPtr curns;
								xmlNodePtr nsparent;
								
								nsparent = node->_private;
								curns = xmlNewNs(NULL, node->name, NULL);
								if (node->children) {
									curns->prefix = xmlStrdup((char *) node->children);
								}
								if (node->children) {
									node = xmlNewDocNode(node->doc, NULL, (char *) node->children, node->name);
								} else {
									node = xmlNewDocNode(node->doc, NULL, "xmlns", node->name);
								}
								node->type = XML_NAMESPACE_DECL;
								node->parent = nsparent;
								node->ns = curns;
							} else {
								node = xmlDocCopyNodeList(domintern->document->ptr, node);
							}

							child = php_dom_create_object(node, &ret, child, domintern TSRMLS_CC);
							add_next_index_zval(args[i], child);
						}
					}
				}
				break;
			default:
				str = xmlXPathCastToString(obj);
				ZVAL_STRING(args[i], str, 1);
				xmlFree(str);
		}
		xmlXPathFreeObject(obj);
		fci.params[i] = &args[i];
	}
	
	fci.size = sizeof(fci);
	fci.function_table = EG(function_table);
	
	obj = valuePop(ctxt);
	if (obj->stringval == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Handler name must be a string");
		xmlXPathFreeObject(obj);
		valuePush(ctxt, xmlXPathNewString(""));
		if (fci.param_count > 0) {
			for (i = 0; i < nargs - 1; i++) {
				zval_ptr_dtor(&args[i]);
			}
			efree(args);
			efree(fci.params);
		}
		return; 
	}
	INIT_PZVAL(&handler);
	ZVAL_STRING(&handler, obj->stringval, 1);
	xmlXPathFreeObject(obj);
	
	fci.function_name = &handler;
	fci.symbol_table = NULL;
	fci.object_ptr = NULL;
	fci.retval_ptr_ptr = &retval;
	fci.no_separation = 0;
	/*fci.function_handler_cache = &function_ptr;*/
	if (!zend_make_callable(&handler, &callable TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to call handler %s()", callable);
		valuePush(ctxt, xmlXPathNewString(""));
	} else if ( intern->registerPhpFunctions == 2 && zend_hash_exists(intern->registered_phpfunctions, callable, strlen(callable) + 1) == 0) { 
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not allowed to call handler '%s()'", callable);
		/* Push an empty string, so that we at least have an xslt result... */
		valuePush(ctxt, xmlXPathNewString(""));
	} else {
		result = zend_call_function(&fci, NULL TSRMLS_CC);
		if (result == FAILURE) {
			if (Z_TYPE(handler) == IS_STRING) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to call handler %s()", Z_STRVAL_P(&handler));
				valuePush(ctxt, xmlXPathNewString(""));
			}
		/* retval is == NULL, when an exception occurred, don't report anything, because PHP itself will handle that */
		} else if (retval == NULL) {
		} else {
			if (retval->type == IS_OBJECT && instanceof_function( Z_OBJCE_P(retval), dom_node_class_entry TSRMLS_CC)) {
				xmlNode *nodep;
				dom_object *obj;
				if (intern->node_list == NULL) {
					ALLOC_HASHTABLE(intern->node_list);
					zend_hash_init(intern->node_list, 0, NULL, ZVAL_PTR_DTOR, 0);
				}
				zval_add_ref(&retval);
				zend_hash_next_index_insert(intern->node_list, &retval, sizeof(zval *), NULL);
				obj = (dom_object *)zend_object_store_get_object(retval TSRMLS_CC);
				nodep = dom_object_get_node(obj);
				valuePush(ctxt, xmlXPathNewNodeSet(nodep));
			} else if (retval->type == IS_BOOL) {
				valuePush(ctxt, xmlXPathNewBoolean(retval->value.lval));
			} else if (retval->type == IS_OBJECT) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "A PHP Object cannot be converted to a XPath-string");
				valuePush(ctxt, xmlXPathNewString(""));
			} else {
				convert_to_string_ex(&retval);
				valuePush(ctxt, xmlXPathNewString( Z_STRVAL_P(retval)));
			}
			zval_ptr_dtor(&retval);
		}
	}
	efree(callable);
	zval_dtor(&handler);
	if (fci.param_count > 0) {
		for (i = 0; i < nargs - 1; i++) {
			zval_ptr_dtor(&args[i]);
		}
		efree(args);
		efree(fci.params);
	}
}