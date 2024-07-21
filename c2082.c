CPH_METHOD(LoadFromFile)
{
	HRESULT res;
	char *filename, *fullpath;
	int filename_len;
	long flags = 0;
	OLECHAR *olefilename;
	CPH_FETCH();
	
	CPH_NO_OBJ();

	res = get_persist_file(helper);
	if (helper->ipf) {

		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l",
					&filename, &filename_len, &flags)) {
			php_com_throw_exception(E_INVALIDARG, "Invalid arguments" TSRMLS_CC);
			return;
		}

		if (!(fullpath = expand_filepath(filename, NULL TSRMLS_CC))) {
			RETURN_FALSE;
		}

		if ((PG(safe_mode) && (!php_checkuid(fullpath, NULL, CHECKUID_CHECK_FILE_AND_DIR))) ||
				php_check_open_basedir(fullpath TSRMLS_CC)) {
			efree(fullpath);
			RETURN_FALSE;
		}

		olefilename = php_com_string_to_olestring(fullpath, strlen(fullpath), helper->codepage TSRMLS_CC);
		efree(fullpath);
			
		res = IPersistFile_Load(helper->ipf, olefilename, flags);
		efree(olefilename);

		if (FAILED(res)) {
			php_com_throw_exception(res, NULL TSRMLS_CC);
		}
		
	} else {
		php_com_throw_exception(res, NULL TSRMLS_CC);
	}
}