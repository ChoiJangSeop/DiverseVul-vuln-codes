static size_t curl_progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	php_curl       *ch = (php_curl *) clientp;
	php_curl_progress  *t  = ch->handlers->progress;
	size_t	rval = 0;

#if PHP_CURL_DEBUG
	fprintf(stderr, "curl_progress() called\n");
	fprintf(stderr, "clientp = %x, dltotal = %f, dlnow = %f, ultotal = %f, ulnow = %f\n", clientp, dltotal, dlnow, ultotal, ulnow);
#endif

	switch (t->method) {
		case PHP_CURL_USER: {
			zval **argv[4];
			zval  *zdltotal = NULL;
			zval  *zdlnow = NULL;
			zval  *zultotal = NULL;
			zval  *zulnow = NULL;
			zval  *retval_ptr;
			int   error;
			zend_fcall_info fci;
			TSRMLS_FETCH_FROM_CTX(ch->thread_ctx);

			MAKE_STD_ZVAL(zdltotal);
			MAKE_STD_ZVAL(zdlnow);
			MAKE_STD_ZVAL(zultotal);
			MAKE_STD_ZVAL(zulnow);

			ZVAL_LONG(zdltotal, (long) dltotal);
			ZVAL_LONG(zdlnow, (long) dlnow);
			ZVAL_LONG(zultotal, (long) ultotal);
			ZVAL_LONG(zulnow, (long) ulnow);

			argv[0] = &zdltotal;
			argv[1] = &zdlnow;
			argv[2] = &zultotal;
			argv[3] = &zulnow;

			fci.size = sizeof(fci);
			fci.function_table = EG(function_table);
			fci.function_name = t->func_name;
			fci.object_ptr = NULL;
			fci.retval_ptr_ptr = &retval_ptr;
			fci.param_count = 4;
			fci.params = argv;
			fci.no_separation = 0;
			fci.symbol_table = NULL;

			ch->in_callback = 1;
			error = zend_call_function(&fci, &t->fci_cache TSRMLS_CC);
			ch->in_callback = 0;
			if (error == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot call the CURLOPT_PROGRESSFUNCTION");
			} else if (retval_ptr) {
				if (Z_TYPE_P(retval_ptr) != IS_LONG) {
					convert_to_long_ex(&retval_ptr);
				}
				if (0 != Z_LVAL_P(retval_ptr)) {
					rval = 1;
				}
				zval_ptr_dtor(&retval_ptr);
			}
			zval_ptr_dtor(argv[0]);
			zval_ptr_dtor(argv[1]);
			zval_ptr_dtor(argv[2]);
			zval_ptr_dtor(argv[3]);
			break;
		}
	}
	return rval;
}