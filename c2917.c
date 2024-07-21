int spl_object_storage_detach(spl_SplObjectStorage *intern, zval *this, zval *obj TSRMLS_DC) /* {{{ */
{
	int hash_len, ret = FAILURE;
	char *hash = spl_object_storage_get_hash(intern, this, obj, &hash_len TSRMLS_CC);
	if (!hash) {
		return ret;
	}
	ret = zend_hash_del(&intern->storage, hash, hash_len);
	spl_object_storage_free_hash(intern, hash);
	
	return ret;
} /* }}}*/