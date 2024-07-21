SAPI_API SAPI_TREAT_DATA_FUNC(php_default_treat_data)
{
	char *res = NULL, *var, *val, *separator = NULL;
	const char *c_var;
	zval *array_ptr;
	int free_buffer = 0;
	char *strtok_buf = NULL;
	long count = 0;
	
	switch (arg) {
		case PARSE_POST:
		case PARSE_GET:
		case PARSE_COOKIE:
			ALLOC_ZVAL(array_ptr);
			array_init(array_ptr);
			INIT_PZVAL(array_ptr);
			switch (arg) {
				case PARSE_POST:
					if (PG(http_globals)[TRACK_VARS_POST]) {
						zval_ptr_dtor(&PG(http_globals)[TRACK_VARS_POST]);
					}
					PG(http_globals)[TRACK_VARS_POST] = array_ptr;
					break;
				case PARSE_GET:
					if (PG(http_globals)[TRACK_VARS_GET]) {
						zval_ptr_dtor(&PG(http_globals)[TRACK_VARS_GET]);
					}
					PG(http_globals)[TRACK_VARS_GET] = array_ptr;
					break;
				case PARSE_COOKIE:
					if (PG(http_globals)[TRACK_VARS_COOKIE]) {
						zval_ptr_dtor(&PG(http_globals)[TRACK_VARS_COOKIE]);
					}
					PG(http_globals)[TRACK_VARS_COOKIE] = array_ptr;
					break;
			}
			break;
		default:
			array_ptr = destArray;
			break;
	}

	if (arg == PARSE_POST) {
		sapi_handle_post(array_ptr TSRMLS_CC);
		return;
	}

	if (arg == PARSE_GET) {		/* GET data */
		c_var = SG(request_info).query_string;
		if (c_var && *c_var) {
			res = (char *) estrdup(c_var);
			free_buffer = 1;
		} else {
			free_buffer = 0;
		}
	} else if (arg == PARSE_COOKIE) {		/* Cookie data */
		c_var = SG(request_info).cookie_data;
		if (c_var && *c_var) {
			res = (char *) estrdup(c_var);
			free_buffer = 1;
		} else {
			free_buffer = 0;
		}
	} else if (arg == PARSE_STRING) {		/* String data */
		res = str;
		free_buffer = 1;
	}

	if (!res) {
		return;
	}

	switch (arg) {
		case PARSE_GET:
		case PARSE_STRING:
			separator = (char *) estrdup(PG(arg_separator).input);
			break;
		case PARSE_COOKIE:
			separator = ";\0";
			break;
	}
	
	var = php_strtok_r(res, separator, &strtok_buf);
	
	while (var) {
		val = strchr(var, '=');

		if (arg == PARSE_COOKIE) {
			/* Remove leading spaces from cookie names, needed for multi-cookie header where ; can be followed by a space */
			while (isspace(*var)) {
				var++;
			}
			if (var == val || *var == '\0') {
				goto next_cookie;
			}
		}

		if (++count > PG(max_input_vars)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Input variables exceeded %ld. To increase the limit change max_input_vars in php.ini.", PG(max_input_vars));
			break;
		}

		if (val) { /* have a value */
			int val_len;
			unsigned int new_val_len;

			*val++ = '\0';
			php_url_decode(var, strlen(var));
			val_len = php_url_decode(val, strlen(val));
			val = estrndup(val, val_len);
			if (sapi_module.input_filter(arg, var, &val, val_len, &new_val_len TSRMLS_CC)) {
				php_register_variable_safe(var, val, new_val_len, array_ptr TSRMLS_CC);
			}
			efree(val);
		} else {
			int val_len;
			unsigned int new_val_len;

			php_url_decode(var, strlen(var));
			val_len = 0;
			val = estrndup("", val_len);
			if (sapi_module.input_filter(arg, var, &val, val_len, &new_val_len TSRMLS_CC)) {
				php_register_variable_safe(var, val, new_val_len, array_ptr TSRMLS_CC);
			}
			efree(val);
		}
next_cookie:
		var = php_strtok_r(NULL, separator, &strtok_buf);
	}

	if (arg != PARSE_COOKIE) {
		efree(separator);
	}

	if (free_buffer) {
		efree(res);
	}
}