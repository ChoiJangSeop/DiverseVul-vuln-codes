static zend_bool php_auto_globals_create_files(const char *name, uint name_len TSRMLS_DC)
{
	zval *vars;

	if (PG(http_globals)[TRACK_VARS_FILES]) {
		vars = PG(http_globals)[TRACK_VARS_FILES];
	} else {
		ALLOC_ZVAL(vars);
		array_init(vars);
		INIT_PZVAL(vars);
		PG(http_globals)[TRACK_VARS_FILES] = vars;
	}

	zend_hash_update(&EG(symbol_table), name, name_len + 1, &vars, sizeof(zval *), NULL);
	Z_ADDREF_P(vars);
	
	return 0; /* don't rearm */
}