static void sapi_run_header_callback(TSRMLS_D)
{
	int   error;
	zend_fcall_info fci;
	char *callback_name = NULL;
	char *callback_error = NULL;
	zval *retval_ptr = NULL;
	
	if (zend_fcall_info_init(SG(callback_func), 0, &fci, &SG(fci_cache), &callback_name, &callback_error TSRMLS_CC) == SUCCESS) {
		fci.retval_ptr_ptr = &retval_ptr;
		
		error = zend_call_function(&fci, &SG(fci_cache) TSRMLS_CC);
		if (error == FAILURE) {
			goto callback_failed;
		} else if (retval_ptr) {
			zval_ptr_dtor(&retval_ptr);
		}
	} else {
callback_failed:
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not call the sapi_header_callback");
	}
	
	if (callback_name) {
		efree(callback_name);
	}
	if (callback_error) {
		efree(callback_error);
	}	
}