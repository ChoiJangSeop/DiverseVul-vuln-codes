void spl_SplOjectStorage_free_storage(void *object TSRMLS_DC) /* {{{ */
{
	spl_SplObjectStorage *intern = (spl_SplObjectStorage *)object;

	zend_object_std_dtor(&intern->std TSRMLS_CC);
	
	zend_hash_destroy(&intern->storage);
	
	if (intern->debug_info != NULL) {
		zend_hash_destroy(intern->debug_info);
		efree(intern->debug_info);
	}

	efree(object);
} /* }}} */