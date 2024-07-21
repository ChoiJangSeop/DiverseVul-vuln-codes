static zend_object_value spl_object_storage_new_ex(zend_class_entry *class_type, spl_SplObjectStorage **obj, zval *orig TSRMLS_DC) /* {{{ */
{
	zend_object_value     retval;
	spl_SplObjectStorage *intern;
	zend_class_entry     *parent = class_type;

	intern = emalloc(sizeof(spl_SplObjectStorage));
	memset(intern, 0, sizeof(spl_SplObjectStorage));
	*obj = intern;

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
	object_properties_init(&intern->std, class_type);

	zend_hash_init(&intern->storage, 0, NULL, (void (*)(void *))spl_object_storage_dtor, 0);

	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t) spl_SplOjectStorage_free_storage, NULL TSRMLS_CC);
	retval.handlers = &spl_handler_SplObjectStorage;

	if (orig) {
		spl_SplObjectStorage *other = (spl_SplObjectStorage*)zend_object_store_get_object(orig TSRMLS_CC);
		spl_object_storage_addall(intern, orig, other TSRMLS_CC);
	}

	while (parent) {
		if (parent == spl_ce_SplObjectStorage) {
			if (class_type != spl_ce_SplObjectStorage) {
				zend_hash_find(&class_type->function_table, "gethash",    sizeof("gethash"),    (void **) &intern->fptr_get_hash);
				if (intern->fptr_get_hash->common.scope == spl_ce_SplObjectStorage) {
					intern->fptr_get_hash = NULL;
				}
			}
			break;
		}

		parent = parent->parent;
	}

	return retval;
}