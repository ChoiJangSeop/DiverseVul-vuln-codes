PHP_FUNCTION(pcntl_signal)
{
	zval *handle, **dest_handle = NULL;
	char *func_name;
	long signo;
	zend_bool restart_syscalls = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz|b", &signo, &handle, &restart_syscalls) == FAILURE) {
		return;
	}

	if (signo < 1 || signo > 32) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid signal");
		RETURN_FALSE;
	}

	if (!PCNTL_G(spares)) {
		/* since calling malloc() from within a signal handler is not portable,
		 * pre-allocate a few records for recording signals */
		int i;
		for (i = 0; i < 32; i++) {
			struct php_pcntl_pending_signal *psig;

			psig = emalloc(sizeof(*psig));
			psig->next = PCNTL_G(spares);
			PCNTL_G(spares) = psig;
		}
	}

	/* Special long value case for SIG_DFL and SIG_IGN */
	if (Z_TYPE_P(handle)==IS_LONG) {
		if (Z_LVAL_P(handle) != (long) SIG_DFL && Z_LVAL_P(handle) != (long) SIG_IGN) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid value for handle argument specified");
			RETURN_FALSE;
		}
		if (php_signal(signo, (Sigfunc *) Z_LVAL_P(handle), (int) restart_syscalls) == SIG_ERR) {
			PCNTL_G(last_error) = errno;
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error assigning signal");
			RETURN_FALSE;
		}
		RETURN_TRUE;
	}
	
	if (!zend_is_callable(handle, 0, &func_name TSRMLS_CC)) {
		PCNTL_G(last_error) = EINVAL;
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s is not a callable function name error", func_name);
		efree(func_name);
		RETURN_FALSE;
	}
	efree(func_name);
	
	/* Add the function name to our signal table */
	zend_hash_index_update(&PCNTL_G(php_signal_table), signo, (void **) &handle, sizeof(zval *), (void **) &dest_handle);
	if (dest_handle) zval_add_ref(dest_handle);
	
	if (php_signal4(signo, pcntl_signal_handler, (int) restart_syscalls, 1) == SIG_ERR) {
		PCNTL_G(last_error) = errno;
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error assigning signal");
		RETURN_FALSE;
	}
	RETURN_TRUE;
}