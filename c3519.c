PHP_FUNCTION(stream_get_meta_data)
{
	zval *arg1;
	php_stream *stream;
	zval *newval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &arg1) == FAILURE) {
		return;
	}
	php_stream_from_zval(stream, &arg1);

	array_init(return_value);

	if (stream->wrapperdata) {
		MAKE_STD_ZVAL(newval);
		MAKE_COPY_ZVAL(&stream->wrapperdata, newval);

		add_assoc_zval(return_value, "wrapper_data", newval);
	}
	if (stream->wrapper) {
		add_assoc_string(return_value, "wrapper_type", (char *)stream->wrapper->wops->label, 1);
	}
	add_assoc_string(return_value, "stream_type", (char *)stream->ops->label, 1);

	add_assoc_string(return_value, "mode", stream->mode, 1);

#if 0	/* TODO: needs updating for new filter API */
	if (stream->filterhead) {
		php_stream_filter *filter;

		MAKE_STD_ZVAL(newval);
		array_init(newval);

		for (filter = stream->filterhead; filter != NULL; filter = filter->next) {
			add_next_index_string(newval, (char *)filter->fops->label, 1);
		}

		add_assoc_zval(return_value, "filters", newval);
	}
#endif

	add_assoc_long(return_value, "unread_bytes", stream->writepos - stream->readpos);

	add_assoc_bool(return_value, "seekable", (stream->ops->seek) && (stream->flags & PHP_STREAM_FLAG_NO_SEEK) == 0);
	if (stream->orig_path) {
		add_assoc_string(return_value, "uri", stream->orig_path, 1);
	}

	if (!php_stream_populate_meta_data(stream, return_value)) {
		add_assoc_bool(return_value, "timed_out", 0);
		add_assoc_bool(return_value, "blocked", 1);
		add_assoc_bool(return_value, "eof", php_stream_eof(stream));
	}

}