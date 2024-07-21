static zend_bool php_auto_globals_create_get(const char *name, uint name_len TSRMLS_DC)
{
	zval *vars;

	if (PG(variables_order) && (strchr(PG(variables_order),'G') || strchr(PG(variables_order),'g'))) {
		sapi_module.treat_data(PARSE_GET, NULL, NULL TSRMLS_CC);
		vars = PG(http_globals)[TRACK_VARS_GET];
	} else {
		ALLOC_ZVAL(vars);
		array_init(vars);
		INIT_PZVAL(vars);
		if (PG(http_globals)[TRACK_VARS_GET]) {
			zval_ptr_dtor(&PG(http_globals)[TRACK_VARS_GET]);
		}
		PG(http_globals)[TRACK_VARS_GET] = vars;
	}

	zend_hash_update(&EG(symbol_table), name, name_len + 1, &vars, sizeof(zval *), NULL);
	Z_ADDREF_P(vars);
	
	return 0; /* don't rearm */
}