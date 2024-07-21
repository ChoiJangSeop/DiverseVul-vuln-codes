void spl_object_storage_attach(spl_SplObjectStorage *intern, zval *this, zval *obj, zval *inf TSRMLS_DC) /* {{{ */
{
	spl_SplObjectStorageElement *pelement, element;
	
	int hash_len;
	char *hash = spl_object_storage_get_hash(intern, this, obj, &hash_len TSRMLS_CC);
	if (!hash) {
		return;
	}

	pelement = spl_object_storage_get(intern, hash, hash_len TSRMLS_CC);

	if (inf) {
		Z_ADDREF_P(inf);
	} else {
		ALLOC_INIT_ZVAL(inf);
	}
	if (pelement) {
		zval_ptr_dtor(&pelement->inf);
		pelement->inf = inf;
		spl_object_storage_free_hash(intern, hash);
		return;
	}
	Z_ADDREF_P(obj);
	element.obj = obj;
	element.inf = inf;
	zend_hash_update(&intern->storage, hash, hash_len, &element, sizeof(spl_SplObjectStorageElement), NULL);
	spl_object_storage_free_hash(intern, hash);
} /* }}} */