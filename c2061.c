	}
}

static TIDY_DOC_METHOD(parseFile)
{
	char *inputfile, *enc = NULL;
	int input_len, enc_len = 0, contents_len = 0;
	zend_bool use_include_path = 0;
	char *contents;
	zval **options = NULL;
	PHPTidyObj *obj;

	TIDY_SET_CONTEXT;

	obj = (PHPTidyObj *)zend_object_store_get_object(object TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|Zsb", &inputfile, &input_len,
							  &options, &enc, &enc_len, &use_include_path) == FAILURE) {
		RETURN_FALSE;
	}
	
	if (!(contents = php_tidy_file_to_mem(inputfile, use_include_path, &contents_len TSRMLS_CC))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot Load '%s' into memory %s", inputfile, (use_include_path) ? "(Using include path)" : "");
		RETURN_FALSE;
	}

	TIDY_APPLY_CONFIG_ZVAL(obj->ptdoc->doc, options);

	if(php_tidy_parse_string(obj, contents, contents_len, enc TSRMLS_CC) == FAILURE) {
		RETVAL_FALSE;
	} else {
		RETVAL_TRUE;
	}