static zend_bool php_auto_globals_create_server(const char *name, uint name_len TSRMLS_DC)
{
	if (PG(variables_order) && (strchr(PG(variables_order),'S') || strchr(PG(variables_order),'s'))) {
		php_register_server_variables(TSRMLS_C);

		if (PG(register_argc_argv)) {
			if (SG(request_info).argc) {
				zval **argc, **argv;
	
				if (zend_hash_find(&EG(symbol_table), "argc", sizeof("argc"), (void**)&argc) == SUCCESS &&
					zend_hash_find(&EG(symbol_table), "argv", sizeof("argv"), (void**)&argv) == SUCCESS) {
					Z_ADDREF_PP(argc);
					Z_ADDREF_PP(argv);
					zend_hash_update(Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]), "argv", sizeof("argv"), argv, sizeof(zval *), NULL);
					zend_hash_update(Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]), "argc", sizeof("argc"), argc, sizeof(zval *), NULL);
				}
			} else {
				php_build_argv(SG(request_info).query_string, PG(http_globals)[TRACK_VARS_SERVER] TSRMLS_CC);
			}
		}
	
	} else {
		zval *server_vars=NULL;
		ALLOC_ZVAL(server_vars);
		array_init(server_vars);
		INIT_PZVAL(server_vars);
		if (PG(http_globals)[TRACK_VARS_SERVER]) {
			zval_ptr_dtor(&PG(http_globals)[TRACK_VARS_SERVER]);
		}
		PG(http_globals)[TRACK_VARS_SERVER] = server_vars;
	}

	zend_hash_update(&EG(symbol_table), name, name_len + 1, &PG(http_globals)[TRACK_VARS_SERVER], sizeof(zval *), NULL);
	Z_ADDREF_P(PG(http_globals)[TRACK_VARS_SERVER]);
	
	return 0; /* don't rearm */
}