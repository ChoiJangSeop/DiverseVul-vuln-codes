PHP_METHOD(SoapServer, SoapServer)
{
	soapServicePtr service;
	zval *wsdl = NULL, *options = NULL;
	int ret;
	int version = SOAP_1_1;
	long cache_wsdl;
	HashTable *typemap_ht = NULL;

	SOAP_SERVER_BEGIN_CODE();

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &wsdl, &options) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
	}

	if (Z_TYPE_P(wsdl) != IS_STRING && Z_TYPE_P(wsdl) != IS_NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
	}
	
	service = emalloc(sizeof(soapService));
	memset(service, 0, sizeof(soapService));
	service->send_errors = 1;

	cache_wsdl = SOAP_GLOBAL(cache_enabled) ? SOAP_GLOBAL(cache_mode) : 0;

	if (options != NULL) {
		HashTable *ht = Z_ARRVAL_P(options);
		zval **tmp;

		if (zend_hash_find(ht, "soap_version", sizeof("soap_version"), (void**)&tmp) == SUCCESS) {
			if (Z_TYPE_PP(tmp) == IS_LONG &&
			    (Z_LVAL_PP(tmp) == SOAP_1_1 || Z_LVAL_PP(tmp) == SOAP_1_2)) {
				version = Z_LVAL_PP(tmp);
			} else {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "'soap_version' option must be SOAP_1_1 or SOAP_1_2");
			}
		}

		if (zend_hash_find(ht, "uri", sizeof("uri"), (void**)&tmp) == SUCCESS &&
		    Z_TYPE_PP(tmp) == IS_STRING) {
			service->uri = estrndup(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
		} else if (Z_TYPE_P(wsdl) == IS_NULL) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "'uri' option is required in nonWSDL mode");
		}

		if (zend_hash_find(ht, "actor", sizeof("actor"), (void**)&tmp) == SUCCESS &&
		    Z_TYPE_PP(tmp) == IS_STRING) {
			service->actor = estrndup(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
		}

		if (zend_hash_find(ht, "encoding", sizeof("encoding"), (void**)&tmp) == SUCCESS &&
		    Z_TYPE_PP(tmp) == IS_STRING) {
			xmlCharEncodingHandlerPtr encoding;
		
			encoding = xmlFindCharEncodingHandler(Z_STRVAL_PP(tmp));
			if (encoding == NULL) {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid 'encoding' option - '%s'", Z_STRVAL_PP(tmp));
			} else {
			  service->encoding = encoding;
			}
		}

		if (zend_hash_find(ht, "classmap", sizeof("classmap"), (void**)&tmp) == SUCCESS &&
			Z_TYPE_PP(tmp) == IS_ARRAY) {
			zval *ztmp;

			ALLOC_HASHTABLE(service->class_map);
			zend_hash_init(service->class_map, zend_hash_num_elements((*tmp)->value.ht), NULL, ZVAL_PTR_DTOR, 0);
			zend_hash_copy(service->class_map, (*tmp)->value.ht, (copy_ctor_func_t) zval_add_ref, (void *) &ztmp, sizeof(zval *));
		}

		if (zend_hash_find(ht, "typemap", sizeof("typemap"), (void**)&tmp) == SUCCESS &&
			Z_TYPE_PP(tmp) == IS_ARRAY &&
			zend_hash_num_elements(Z_ARRVAL_PP(tmp)) > 0) {
			typemap_ht = Z_ARRVAL_PP(tmp);
		}

		if (zend_hash_find(ht, "features", sizeof("features"), (void**)&tmp) == SUCCESS &&
			Z_TYPE_PP(tmp) == IS_LONG) {
			service->features = Z_LVAL_PP(tmp);
		}

		if (zend_hash_find(ht, "cache_wsdl", sizeof("cache_wsdl"), (void**)&tmp) == SUCCESS &&
		    Z_TYPE_PP(tmp) == IS_LONG) {
			cache_wsdl = Z_LVAL_PP(tmp);
		}

		if (zend_hash_find(ht, "send_errors", sizeof("send_errors"), (void**)&tmp) == SUCCESS &&
		    (Z_TYPE_PP(tmp) == IS_BOOL || Z_TYPE_PP(tmp) == IS_LONG)) {
			service->send_errors = Z_LVAL_PP(tmp);
		}

	} else if (Z_TYPE_P(wsdl) == IS_NULL) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "'uri' option is required in nonWSDL mode");
	}

	service->version = version;
	service->type = SOAP_FUNCTIONS;
	service->soap_functions.functions_all = FALSE;
	service->soap_functions.ft = emalloc(sizeof(HashTable));
	zend_hash_init(service->soap_functions.ft, 0, NULL, ZVAL_PTR_DTOR, 0);

	if (Z_TYPE_P(wsdl) != IS_NULL) {
		service->sdl = get_sdl(this_ptr, Z_STRVAL_P(wsdl), cache_wsdl TSRMLS_CC);
		if (service->uri == NULL) {
			if (service->sdl->target_ns) {
				service->uri = estrdup(service->sdl->target_ns);
			} else {
				/*FIXME*/
				service->uri = estrdup("http://unknown-uri/");
			}
		}
	}
	
	if (typemap_ht) {
		service->typemap = soap_create_typemap(service->sdl, typemap_ht TSRMLS_CC);
	}

	ret = zend_list_insert(service, le_service TSRMLS_CC);
	add_property_resource(this_ptr, "service", ret);

	SOAP_SERVER_END_CODE();
}