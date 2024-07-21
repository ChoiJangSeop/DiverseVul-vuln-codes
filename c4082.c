static zend_bool php_auto_globals_create_post(const char *name, uint name_len TSRMLS_DC)
{
	zval *vars;

	if (PG(variables_order) &&
			(strchr(PG(variables_order),'P') || strchr(PG(variables_order),'p')) &&
		!SG(headers_sent) &&
		SG(request_info).request_method &&
		!strcasecmp(SG(request_info).request_method, "POST")) {
		sapi_module.treat_data(PARSE_POST, NULL, NULL TSRMLS_CC);
		vars = PG(http_globals)[TRACK_VARS_POST];
	} else {
		ALLOC_ZVAL(vars);
		array_init(vars);
		INIT_PZVAL(vars);
		if (PG(http_globals)[TRACK_VARS_POST]) {
			zval_ptr_dtor(&PG(http_globals)[TRACK_VARS_POST]);
		}
		PG(http_globals)[TRACK_VARS_POST] = vars;
	}

	zend_hash_update(&EG(symbol_table), name, name_len + 1, &vars, sizeof(zval *), NULL);
	Z_ADDREF_P(vars);
	
	return 0; /* don't rearm */
}