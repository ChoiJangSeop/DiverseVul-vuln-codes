PHP_FUNCTION(xsl_xsltprocessor_transform_to_uri)
{
	zval *id, *docp = NULL;
	xmlDoc *newdocp;
	xsltStylesheetPtr sheetp;
	int ret, uri_len;
	char *uri;
	xsl_object *intern;
	
	id = getThis();
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	sheetp = (xsltStylesheetPtr) intern->ptr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "op", &docp, &uri, &uri_len) == FAILURE) {
		RETURN_FALSE;
	}

	newdocp = php_xsl_apply_stylesheet(id, intern, sheetp, docp TSRMLS_CC);

	ret = -1;
	if (newdocp) {
		ret = xsltSaveResultToFilename(uri, newdocp, sheetp, 0);
		xmlFreeDoc(newdocp);
	}

	RETVAL_LONG(ret);
}