ZEND_METHOD(exception, getTraceAsString)
{
	zval *trace;
	char *res, **str, *s_tmp;
	int res_len = 0, *len = &res_len, num = 0;

	DEFAULT_0_PARAMS;
	
	res = estrdup("");
	str = &res;

	trace = zend_read_property(default_exception_ce, getThis(), "trace", sizeof("trace")-1, 1 TSRMLS_CC);
	zend_hash_apply_with_arguments(Z_ARRVAL_P(trace) TSRMLS_CC, (apply_func_args_t)_build_trace_string, 3, str, len, &num);

	s_tmp = emalloc(1 + MAX_LENGTH_OF_LONG + 7 + 1);
	sprintf(s_tmp, "#%d {main}", num);
	TRACE_APPEND_STRL(s_tmp, strlen(s_tmp));
	efree(s_tmp);

	res[res_len] = '\0';	
	RETURN_STRINGL(res, res_len, 0); 
}