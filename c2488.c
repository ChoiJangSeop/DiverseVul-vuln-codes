
static int php_date_initialize_from_hash(php_date_obj **dateobj, HashTable *myht TSRMLS_DC)
{
	zval            **z_date = NULL;
	zval            **z_timezone = NULL;
	zval            **z_timezone_type = NULL;
	zval             *tmp_obj = NULL;
	timelib_tzinfo   *tzi;
	php_timezone_obj *tzobj;

	if (zend_hash_find(myht, "date", 5, (void**) &z_date) == SUCCESS) {
		convert_to_string(*z_date);
		if (zend_hash_find(myht, "timezone_type", 14, (void**) &z_timezone_type) == SUCCESS) {
			convert_to_long(*z_timezone_type);
			if (zend_hash_find(myht, "timezone", 9, (void**) &z_timezone) == SUCCESS) {
				convert_to_string(*z_timezone);

				switch (Z_LVAL_PP(z_timezone_type)) {
					case TIMELIB_ZONETYPE_OFFSET:
					case TIMELIB_ZONETYPE_ABBR: {
						char *tmp = emalloc(Z_STRLEN_PP(z_date) + Z_STRLEN_PP(z_timezone) + 2);
						int ret;
						snprintf(tmp, Z_STRLEN_PP(z_date) + Z_STRLEN_PP(z_timezone) + 2, "%s %s", Z_STRVAL_PP(z_date), Z_STRVAL_PP(z_timezone));
						ret = php_date_initialize(*dateobj, tmp, Z_STRLEN_PP(z_date) + Z_STRLEN_PP(z_timezone) + 1, NULL, NULL, 0 TSRMLS_CC);
						efree(tmp);
						return 1 == ret;
					}

					case TIMELIB_ZONETYPE_ID: {
						int ret;
						convert_to_string(*z_timezone);

						tzi = php_date_parse_tzfile(Z_STRVAL_PP(z_timezone), DATE_TIMEZONEDB TSRMLS_CC);

						if (tzi == NULL) {
							return 0;
						}

						ALLOC_INIT_ZVAL(tmp_obj);
						tzobj = zend_object_store_get_object(php_date_instantiate(date_ce_timezone, tmp_obj TSRMLS_CC) TSRMLS_CC);
						tzobj->type = TIMELIB_ZONETYPE_ID;
						tzobj->tzi.tz = tzi;
						tzobj->initialized = 1;

						ret = php_date_initialize(*dateobj, Z_STRVAL_PP(z_date), Z_STRLEN_PP(z_date), NULL, tmp_obj, 0 TSRMLS_CC);
						zval_ptr_dtor(&tmp_obj);
						return 1 == ret;
					}
				}
			}
		}
	}
	return 0;