SPL_METHOD(SplObjectStorage, unserialize)
{
	spl_SplObjectStorage *intern = Z_SPLOBJSTORAGE_P(getThis());

	char *buf;
	size_t buf_len;
	const unsigned char *p, *s;
	php_unserialize_data_t var_hash;
	zval entry, inf;
	zval *pcount, *pmembers;
	spl_SplObjectStorageElement *element;
	zend_long count;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &buf, &buf_len) == FAILURE) {
		return;
	}

	if (buf_len == 0) {
		return;
	}

	/* storage */
	s = p = (const unsigned char*)buf;
	PHP_VAR_UNSERIALIZE_INIT(var_hash);

	if (*p!= 'x' || *++p != ':') {
		goto outexcept;
	}
	++p;

	pcount = var_tmp_var(&var_hash);
	if (!php_var_unserialize(pcount, &p, s + buf_len, &var_hash) || Z_TYPE_P(pcount) != IS_LONG) {
		goto outexcept;
	}

	--p; /* for ';' */
	count = Z_LVAL_P(pcount);

	while (count-- > 0) {
		spl_SplObjectStorageElement *pelement;
		zend_string *hash;

		if (*p != ';') {
			goto outexcept;
		}
		++p;
		if(*p != 'O' && *p != 'C' && *p != 'r') {
			goto outexcept;
		}
		/* store reference to allow cross-references between different elements */
		if (!php_var_unserialize(&entry, &p, s + buf_len, &var_hash)) {
			goto outexcept;
		}
		if (Z_TYPE(entry) != IS_OBJECT) {
			zval_ptr_dtor(&entry);
			goto outexcept;
		}
		if (*p == ',') { /* new version has inf */
			++p;
			if (!php_var_unserialize(&inf, &p, s + buf_len, &var_hash)) {
				zval_ptr_dtor(&entry);
				goto outexcept;
			}
		} else {
			ZVAL_UNDEF(&inf);
		}

		hash = spl_object_storage_get_hash(intern, getThis(), &entry);
		if (!hash) {
			zval_ptr_dtor(&entry);
			zval_ptr_dtor(&inf);
			goto outexcept;
		}
		pelement = spl_object_storage_get(intern, hash);
		spl_object_storage_free_hash(intern, hash);
		if (pelement) {
			if (!Z_ISUNDEF(pelement->inf)) {
				var_push_dtor(&var_hash, &pelement->inf);
			}
			if (!Z_ISUNDEF(pelement->obj)) {
				var_push_dtor(&var_hash, &pelement->obj);
			}
		}
		element = spl_object_storage_attach(intern, getThis(), &entry, Z_ISUNDEF(inf)?NULL:&inf);
		var_replace(&var_hash, &entry, &element->obj);
		var_replace(&var_hash, &inf, &element->inf);
		zval_ptr_dtor(&entry);
		ZVAL_UNDEF(&entry);
		zval_ptr_dtor(&inf);
		ZVAL_UNDEF(&inf);
	}

	if (*p != ';') {
		goto outexcept;
	}
	++p;

	/* members */
	if (*p!= 'm' || *++p != ':') {
		goto outexcept;
	}
	++p;

	pmembers = var_tmp_var(&var_hash);
	if (!php_var_unserialize(pmembers, &p, s + buf_len, &var_hash) || Z_TYPE_P(pmembers) != IS_ARRAY) {
		goto outexcept;
	}

	/* copy members */
	object_properties_load(&intern->std, Z_ARRVAL_P(pmembers));

	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
	return;

outexcept:
	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
	zend_throw_exception_ex(spl_ce_UnexpectedValueException, 0, "Error at offset %pd of %d bytes", (zend_long)((char*)p - buf), buf_len);
	return;

} /* }}} */