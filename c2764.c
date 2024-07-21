static void _php_do_opendir(INTERNAL_FUNCTION_PARAMETERS, int createobject)
{
	char *dirname;
	int dir_len;
	zval *zcontext = NULL;
	php_stream_context *context = NULL;
	php_stream *dirp;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|r", &dirname, &dir_len, &zcontext) == FAILURE) {
		RETURN_NULL();
	}

	context = php_stream_context_from_zval(zcontext, 0);
	
	dirp = php_stream_opendir(dirname, REPORT_ERRORS, context);

	if (dirp == NULL) {
		RETURN_FALSE;
	}

	dirp->flags |= PHP_STREAM_FLAG_NO_FCLOSE;
		
	php_set_default_dir(dirp->rsrc_id TSRMLS_CC);

	if (createobject) {
		object_init_ex(return_value, dir_class_entry_ptr);
		add_property_stringl(return_value, "path", dirname, dir_len, 1);
		add_property_resource(return_value, "handle", dirp->rsrc_id);
		php_stream_auto_cleanup(dirp); /* so we don't get warnings under debug */
	} else {
		php_stream_to_zval(dirp, return_value);
	}
}