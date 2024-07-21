CPH_METHOD(SaveToFile)
{
	HRESULT res;
	char *filename, *fullpath = NULL;
	int filename_len;
	zend_bool remember = TRUE;
	OLECHAR *olefilename = NULL;
	CPH_FETCH();
	
	CPH_NO_OBJ();

	res = get_persist_file(helper);
	if (helper->ipf) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s!|b",
					&filename, &filename_len, &remember)) {
			php_com_throw_exception(E_INVALIDARG, "Invalid arguments" TSRMLS_CC);
			return;
		}

		if (filename) {
			fullpath = expand_filepath(filename, NULL TSRMLS_CC);
			if (!fullpath) {
				RETURN_FALSE;
			}
	
			if ((PG(safe_mode) && (!php_checkuid(fullpath, NULL, CHECKUID_CHECK_FILE_AND_DIR))) || 
					php_check_open_basedir(fullpath TSRMLS_CC)) {
				efree(fullpath);
				RETURN_FALSE;
			}

			olefilename = php_com_string_to_olestring(filename, strlen(fullpath), helper->codepage TSRMLS_CC);
			efree(fullpath);
		}
		res = IPersistFile_Save(helper->ipf, olefilename, remember);
		if (SUCCEEDED(res)) {
			if (!olefilename) {
				res = IPersistFile_GetCurFile(helper->ipf, &olefilename);
				if (S_OK == res) {
					IPersistFile_SaveCompleted(helper->ipf, olefilename);
					CoTaskMemFree(olefilename);
					olefilename = NULL;
				}
			} else if (remember) {
				IPersistFile_SaveCompleted(helper->ipf, olefilename);
			}
		}
			
		if (olefilename) {
			efree(olefilename);
		}

		if (FAILED(res)) {
			php_com_throw_exception(res, NULL TSRMLS_CC);
		}

	} else {
		php_com_throw_exception(res, NULL TSRMLS_CC);
	}
}