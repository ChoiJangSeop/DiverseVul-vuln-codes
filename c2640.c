static PHP_FUNCTION(xmlwriter_open_uri)
{
	char *valid_file = NULL;
	xmlwriter_object *intern;
	xmlTextWriterPtr ptr;
	char *source;
	char resolved_path[MAXPATHLEN + 1];
	int source_len;

#ifdef ZEND_ENGINE_2
	zval *this = getThis();
	ze_xmlwriter_object *ze_obj = NULL;
#endif

#ifndef ZEND_ENGINE_2
	xmlOutputBufferPtr out_buffer;
	void *ioctx;
#endif

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &source, &source_len) == FAILURE) {
		return;
	}
	
#ifdef ZEND_ENGINE_2
	if (this) {
		/* We do not use XMLWRITER_FROM_OBJECT, xmlwriter init function here */
		ze_obj = (ze_xmlwriter_object*) zend_object_store_get_object(this TSRMLS_CC); 
	}
#endif

	if (source_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty string as source");
		RETURN_FALSE;
	}

	valid_file = _xmlwriter_get_valid_file_path(source, resolved_path, MAXPATHLEN TSRMLS_CC);
	if (!valid_file) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to resolve file path");
		RETURN_FALSE;
	}

	/* TODO: Fix either the PHP stream or libxml APIs: it can then detect when a given 
		 path is valid and not report out of memory error. Once it is done, remove the
		 directory check in _xmlwriter_get_valid_file_path */
#ifndef ZEND_ENGINE_2
	ioctx = php_xmlwriter_streams_IO_open_write_wrapper(valid_file TSRMLS_CC);
	if (ioctx == NULL) {
		RETURN_FALSE;
	}

	out_buffer = xmlOutputBufferCreateIO(php_xmlwriter_streams_IO_write, 
		php_xmlwriter_streams_IO_close, ioctx, NULL);

	if (out_buffer == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to create output buffer");
		RETURN_FALSE;
	}
	ptr = xmlNewTextWriter(out_buffer);
#else
	ptr = xmlNewTextWriterFilename(valid_file, 0);
#endif

	if (!ptr) {
		RETURN_FALSE;
	}

	intern = emalloc(sizeof(xmlwriter_object));
	intern->ptr = ptr;
	intern->output = NULL;
#ifndef ZEND_ENGINE_2
	intern->uri_output = out_buffer;
#else
	if (this) {
		if (ze_obj->xmlwriter_ptr) {
			xmlwriter_free_resource_ptr(ze_obj->xmlwriter_ptr TSRMLS_CC);
		}
		ze_obj->xmlwriter_ptr = intern;
		RETURN_TRUE;
	} else
#endif
	{
		ZEND_REGISTER_RESOURCE(return_value,intern,le_xmlwriter);
	}
}