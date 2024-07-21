static zend_bool php_auto_globals_create_env(const char *name, uint name_len TSRMLS_DC)
{
	zval *env_vars = NULL;
	ALLOC_ZVAL(env_vars);
	array_init(env_vars);
	INIT_PZVAL(env_vars);
	if (PG(http_globals)[TRACK_VARS_ENV]) {
		zval_ptr_dtor(&PG(http_globals)[TRACK_VARS_ENV]);
	}
	PG(http_globals)[TRACK_VARS_ENV] = env_vars;
	
	if (PG(variables_order) && (strchr(PG(variables_order),'E') || strchr(PG(variables_order),'e'))) {
		php_import_environment_variables(PG(http_globals)[TRACK_VARS_ENV] TSRMLS_CC);
	}

	zend_hash_update(&EG(symbol_table), name, name_len + 1, &PG(http_globals)[TRACK_VARS_ENV], sizeof(zval *), NULL);
	Z_ADDREF_P(PG(http_globals)[TRACK_VARS_ENV]);

	return 0; /* don't rearm */
}