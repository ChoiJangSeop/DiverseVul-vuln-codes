PHP_FUNCTION(xsl_xsltprocessor_transform_to_xml)
{
	zval *id, *docp = NULL;
	xmlDoc *newdocp;
	xsltStylesheetPtr sheetp;
	int ret;
	xmlChar *doc_txt_ptr;
	int doc_txt_len;
	xsl_object *intern;
	
	id = getThis();
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	sheetp = (xsltStylesheetPtr) intern->ptr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &docp) == FAILURE) {
		RETURN_FALSE;
	}

	newdocp = php_xsl_apply_stylesheet(id, intern, sheetp, docp TSRMLS_CC);

	ret = -1;
	if (newdocp) {
		ret = xsltSaveResultToString(&doc_txt_ptr, &doc_txt_len, newdocp, sheetp);
		if (doc_txt_ptr && doc_txt_len) {
			RETVAL_STRINGL(doc_txt_ptr, doc_txt_len, 1);
			xmlFree(doc_txt_ptr);
		}
		xmlFreeDoc(newdocp);
	}

	if (ret < 0) {
		RETURN_FALSE;
	}
}