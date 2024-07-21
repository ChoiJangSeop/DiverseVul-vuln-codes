static zend_bool php_auto_globals_create_cookie(const char *name, uint name_len TSRMLS_DC)
{
	zval *vars;

	if (PG(variables_order) && (strchr(PG(variables_order),'C') || strchr(PG(variables_order),'c'))) {
		sapi_module.treat_data(PARSE_COOKIE, NULL, NULL TSRMLS_CC);
		vars = PG(http_globals)[TRACK_VARS_COOKIE];
	} else {
		ALLOC_ZVAL(vars);
		array_init(vars);
		INIT_PZVAL(vars);
		if (PG(http_globals)[TRACK_VARS_COOKIE]) {
			zval_ptr_dtor(&PG(http_globals)[TRACK_VARS_COOKIE]);
		}
		PG(http_globals)[TRACK_VARS_COOKIE] = vars;
	}

	zend_hash_update(&EG(symbol_table), name, name_len + 1, &vars, sizeof(zval *), NULL);
	Z_ADDREF_P(vars);
	
	return 0; /* don't rearm */
}