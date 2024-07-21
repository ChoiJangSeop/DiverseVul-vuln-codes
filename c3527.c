PHP_METHOD(PharFileInfo, getContent)
{
	char *error;
	php_stream *fp;
	phar_entry_info *link;

	PHAR_ENTRY_OBJECT();

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (entry_obj->ent.entry->is_dir) {
		zend_throw_exception_ex(spl_ce_BadMethodCallException, 0 TSRMLS_CC,
			"Phar error: Cannot retrieve contents, \"%s\" in phar \"%s\" is a directory", entry_obj->ent.entry->filename, entry_obj->ent.entry->phar->fname);
		return;
	}

	link = phar_get_link_source(entry_obj->ent.entry TSRMLS_CC);

	if (!link) {
		link = entry_obj->ent.entry;
	}

	if (SUCCESS != phar_open_entry_fp(link, &error, 0 TSRMLS_CC)) {
		zend_throw_exception_ex(spl_ce_BadMethodCallException, 0 TSRMLS_CC,
			"Phar error: Cannot retrieve contents, \"%s\" in phar \"%s\": %s", entry_obj->ent.entry->filename, entry_obj->ent.entry->phar->fname, error);
		efree(error);
		return;
	}

	if (!(fp = phar_get_efp(link, 0 TSRMLS_CC))) {
		zend_throw_exception_ex(spl_ce_BadMethodCallException, 0 TSRMLS_CC,
			"Phar error: Cannot retrieve contents of \"%s\" in phar \"%s\"", entry_obj->ent.entry->filename, entry_obj->ent.entry->phar->fname);
		return;
	}

	phar_seek_efp(link, 0, SEEK_SET, 0, 0 TSRMLS_CC);
	Z_TYPE_P(return_value) = IS_STRING;
	Z_STRLEN_P(return_value) = php_stream_copy_to_mem(fp, &(Z_STRVAL_P(return_value)), link->uncompressed_filesize, 0);

	if (!Z_STRVAL_P(return_value)) {
		Z_STRVAL_P(return_value) = estrndup("", 0);
	}
}