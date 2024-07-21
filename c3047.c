PHP_FUNCTION(xsl_xsltprocessor_transform_to_doc)
{
	zval *id, *docp = NULL;
	xmlDoc *newdocp;
	xsltStylesheetPtr sheetp;
	int ret, ret_class_len=0;
	char *ret_class = NULL;
	xsl_object *intern;

	id = getThis();
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	sheetp = (xsltStylesheetPtr) intern->ptr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o|s!", &docp, &ret_class, &ret_class_len) == FAILURE) {
		RETURN_FALSE;
	}

	newdocp = php_xsl_apply_stylesheet(id, intern, sheetp, docp TSRMLS_CC);

	if (newdocp) {
		if (ret_class) {
			int found;
			char *curclass_name;
			zend_class_entry *curce, **ce;
			php_libxml_node_object *interndoc;

			curce = Z_OBJCE_P(docp);
			curclass_name = curce->name;
			while (curce->parent != NULL) {
				curce = curce->parent;
			}

			found = zend_lookup_class(ret_class, ret_class_len, &ce TSRMLS_CC);
			if ((found != SUCCESS) || !instanceof_function(*ce, curce TSRMLS_CC)) {
				xmlFreeDoc(newdocp);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, 
					"Expecting class compatible with %s, '%s' given", curclass_name, ret_class);
				RETURN_FALSE;
			}

			object_init_ex(return_value, *ce);
		
			interndoc = (php_libxml_node_object *)zend_objects_get_address(return_value TSRMLS_CC);
			php_libxml_increment_doc_ref(interndoc, newdocp TSRMLS_CC);
			php_libxml_increment_node_ptr(interndoc, (xmlNodePtr)newdocp, (void *)interndoc TSRMLS_CC);
		} else {
			DOM_RET_OBJ((xmlNodePtr) newdocp, &ret, NULL);
		}
	} else {
		RETURN_FALSE;
	}
	
}