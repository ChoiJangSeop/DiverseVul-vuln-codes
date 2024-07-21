static int php_object_element_export(zval **zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) /* {{{ */
{
	int level;
	smart_str *buf;

	level = va_arg(args, int);
	buf = va_arg(args, smart_str *);

	buffer_append_spaces(buf, level + 2);
	if (hash_key->nKeyLength != 0) {
		const char *class_name; /* ignored, but must be passed to unmangle */
		const char *pname;
		char *pname_esc;
		int  pname_esc_len;
		
		zend_unmangle_property_name(hash_key->arKey, hash_key->nKeyLength - 1,
				&class_name, &pname);
		pname_esc = php_addcslashes(pname, strlen(pname), &pname_esc_len, 0,
			"'\\", 2 TSRMLS_CC);

		smart_str_appendc(buf, '\'');
		smart_str_appendl(buf, pname_esc, pname_esc_len);
		smart_str_appendc(buf, '\'');
		efree(pname_esc);
	} else {
		smart_str_append_long(buf, (long) hash_key->h);
	}
	smart_str_appendl(buf, " => ", 4);
	php_var_export_ex(zv, level + 2, buf TSRMLS_CC);
	smart_str_appendc(buf, ',');
	smart_str_appendc(buf, '\n');
	return 0;
}