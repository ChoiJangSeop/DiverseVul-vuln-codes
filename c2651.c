static size_t curl_read(char *data, size_t size, size_t nmemb, void *ctx)
{
	php_curl       *ch = (php_curl *) ctx;
	php_curl_read  *t  = ch->handlers->read;
	int             length = 0;

	switch (t->method) {
		case PHP_CURL_DIRECT:
			if (t->fp) {
				length = fread(data, size, nmemb, t->fp);
			}
			break;
		case PHP_CURL_USER: {
			zval **argv[3];
			zval  *handle = NULL;
			zval  *zfd = NULL;
			zval  *zlength = NULL;
			zval  *retval_ptr;
			int   error;
			zend_fcall_info fci;
			TSRMLS_FETCH_FROM_CTX(ch->thread_ctx);

			MAKE_STD_ZVAL(handle);
			MAKE_STD_ZVAL(zfd);
			MAKE_STD_ZVAL(zlength);

			ZVAL_RESOURCE(handle, ch->id);
			zend_list_addref(ch->id);
			ZVAL_RESOURCE(zfd, t->fd);
			zend_list_addref(t->fd);
			ZVAL_LONG(zlength, (int) size * nmemb);

			argv[0] = &handle;
			argv[1] = &zfd;
			argv[2] = &zlength;

			fci.size = sizeof(fci);
			fci.function_table = EG(function_table);
			fci.function_name = t->func_name;
			fci.object_ptr = NULL;
			fci.retval_ptr_ptr = &retval_ptr;
			fci.param_count = 3;
			fci.params = argv;
			fci.no_separation = 0;
			fci.symbol_table = NULL;

			ch->in_callback = 1;
			error = zend_call_function(&fci, &t->fci_cache TSRMLS_CC);
			ch->in_callback = 0;
			if (error == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot call the CURLOPT_READFUNCTION");
#if LIBCURL_VERSION_NUM >= 0x070c01 /* 7.12.1 */
				length = CURL_READFUNC_ABORT;
#endif
			} else if (retval_ptr) {
				if (Z_TYPE_P(retval_ptr) == IS_STRING) {
					length = MIN((int) (size * nmemb), Z_STRLEN_P(retval_ptr));
					memcpy(data, Z_STRVAL_P(retval_ptr), length);
				}
				zval_ptr_dtor(&retval_ptr);
			}

			zval_ptr_dtor(argv[0]);
			zval_ptr_dtor(argv[1]);
			zval_ptr_dtor(argv[2]);
			break;
		}
	}

	return length;
}