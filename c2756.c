static HashTable* spl_object_storage_debug_info(zval *obj, int *is_temp TSRMLS_DC) /* {{{ */
{
	spl_SplObjectStorage *intern = (spl_SplObjectStorage*)zend_object_store_get_object(obj TSRMLS_CC);
	spl_SplObjectStorageElement *element;
	HashTable *props;
	HashPosition pos;
	zval *tmp, *storage;
	char md5str[33];
	int name_len;
	char *zname;

	*is_temp = 0;

	props = Z_OBJPROP_P(obj);
	zend_hash_del(props, "\x00gcdata", sizeof("\x00gcdata"));

	if (intern->debug_info == NULL) {
		ALLOC_HASHTABLE(intern->debug_info);
		ZEND_INIT_SYMTABLE_EX(intern->debug_info, zend_hash_num_elements(props) + 1, 0);
	}

	if (intern->debug_info->nApplyCount == 0) {
		zend_hash_copy(intern->debug_info, props, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

		MAKE_STD_ZVAL(storage);
		array_init(storage);

		zend_hash_internal_pointer_reset_ex(&intern->storage, &pos);
		while (zend_hash_get_current_data_ex(&intern->storage, (void **)&element, &pos) == SUCCESS) {
				php_spl_object_hash(element->obj, md5str TSRMLS_CC);
				MAKE_STD_ZVAL(tmp);
				array_init(tmp);
				/* Incrementing the refcount of obj and inf would confuse the garbage collector.
				 * Prefer to null the destructor */
				Z_ARRVAL_P(tmp)->pDestructor = NULL;
				add_assoc_zval_ex(tmp, "obj", sizeof("obj"), element->obj);
				add_assoc_zval_ex(tmp, "inf", sizeof("inf"), element->inf);
				add_assoc_zval_ex(storage, md5str, 33, tmp);
				zend_hash_move_forward_ex(&intern->storage, &pos);
		}

		zname = spl_gen_private_prop_name(spl_ce_SplObjectStorage, "storage", sizeof("storage")-1, &name_len TSRMLS_CC);
		zend_symtable_update(intern->debug_info, zname, name_len+1, &storage, sizeof(zval *), NULL);
		efree(zname);
	}

	return intern->debug_info;
}