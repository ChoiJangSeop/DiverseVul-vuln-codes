static int php_array_element_export(zval **zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) /* {{{ */
{
	int level;
	smart_str *buf;

	level = va_arg(args, int);
	buf = va_arg(args, smart_str *);

	if (hash_key->nKeyLength == 0) { /* numeric key */
		buffer_append_spaces(buf, level+1);
		smart_str_append_long(buf, (long) hash_key->h);
		smart_str_appendl(buf, " => ", 4);

	} else { /* string key */
		char *key, *tmp_str;
		int key_len, tmp_len;
		key = php_addcslashes(hash_key->arKey, hash_key->nKeyLength - 1, &key_len, 0, "'\\", 2 TSRMLS_CC);
		tmp_str = php_str_to_str_ex(key, key_len, "\0", 1, "' . \"\\0\" . '", 12, &tmp_len, 0, NULL);

		buffer_append_spaces(buf, level + 1);

		smart_str_appendc(buf, '\'');
		smart_str_appendl(buf, tmp_str, tmp_len);
		smart_str_appendl(buf, "' => ", 5);

		efree(key);
		efree(tmp_str);
	}
	php_var_export_ex(zv, level + 2, buf TSRMLS_CC);

	smart_str_appendc(buf, ',');
	smart_str_appendc(buf, '\n');
	
	return 0;
}