static void php_set_default_dir(int id TSRMLS_DC)
{
	if (DIRG(default_dir)!=-1) {
		zend_list_delete(DIRG(default_dir));
	}

	if (id != -1) {
		zend_list_addref(id);
	}
	
	DIRG(default_dir) = id;
}