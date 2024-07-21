SAPI_API SAPI_POST_HANDLER_FUNC(php_std_post_handler)
{
	char *var, *val, *e, *s, *p;
	zval *array_ptr = (zval *) arg;
	long count = 0;

	if (SG(request_info).post_data == NULL) {
		return;
	}	

	s = SG(request_info).post_data;
	e = s + SG(request_info).post_data_length;

	while (s < e && (p = memchr(s, '&', (e - s)))) {
last_value:
		if ((val = memchr(s, '=', (p - s)))) { /* have a value */
			unsigned int val_len, new_val_len;

			if (++count > PG(max_input_vars)) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Input variables exceeded %ld. To increase the limit change max_input_vars in php.ini.", PG(max_input_vars));
				return;
			}
			var = s;

			php_url_decode(var, (val - s));
			val++;
			val_len = php_url_decode(val, (p - val));
			val = estrndup(val, val_len);
			if (sapi_module.input_filter(PARSE_POST, var, &val, val_len, &new_val_len TSRMLS_CC)) {
				php_register_variable_safe(var, val, new_val_len, array_ptr TSRMLS_CC);
			}
			efree(val);
		}
		s = p + 1;
	}
	if (s < e) {
		p = e;
		goto last_value;
	}
}