static ZIPARCHIVE_METHOD(addFromString)
{
	struct zip *intern;
	zval *this = getThis();
	char *buffer, *name;
	int buffer_len, name_len;
	ze_zip_object *ze_obj;
	struct zip_source *zs;
	int pos = 0;
	int cur_idx;

	if (!this) {
		RETURN_FALSE;
	}

	ZIP_FROM_OBJECT(intern, this);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
			&name, &name_len, &buffer, &buffer_len) == FAILURE) {
		return;
	}

	ze_obj = (ze_zip_object*) zend_object_store_get_object(this TSRMLS_CC);
	if (ze_obj->buffers_cnt) {
		ze_obj->buffers = (char **)erealloc(ze_obj->buffers, sizeof(char *) * (ze_obj->buffers_cnt+1));
		pos = ze_obj->buffers_cnt++;
	} else {
		ze_obj->buffers = (char **)emalloc(sizeof(char *));
		ze_obj->buffers_cnt++;
		pos = 0;
	}
	ze_obj->buffers[pos] = (char *)emalloc(buffer_len + 1);
	memcpy(ze_obj->buffers[pos], buffer, buffer_len + 1);

	zs = zip_source_buffer(intern, ze_obj->buffers[pos], buffer_len, 0);

	if (zs == NULL) {
		RETURN_FALSE;
	}

	cur_idx = zip_name_locate(intern, (const char *)name, 0);
	/* TODO: fix  _zip_replace */
	if (cur_idx >= 0) {
		if (zip_delete(intern, cur_idx) == -1) {
			goto fail;
		}
	}

	if (zip_add(intern, name, zs) != -1) {
		RETURN_TRUE;
	}
fail:
	zip_source_free(zs);
	RETURN_FALSE;	
}