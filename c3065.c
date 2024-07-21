static PHP_MSHUTDOWN_FUNCTION(zip)
{
#ifdef PHP_ZIP_USE_OO 
	zend_hash_destroy(&zip_prop_handlers);
	php_unregister_url_stream_wrapper("zip" TSRMLS_CC);
#endif
	return SUCCESS;
}