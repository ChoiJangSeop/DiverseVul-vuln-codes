static int php_session_rfc1867_callback(unsigned int event, void *event_data, void **extra TSRMLS_DC) /* {{{ */
{
	php_session_rfc1867_progress *progress;
	int retval = SUCCESS;

	if (php_session_rfc1867_orig_callback) {
		retval = php_session_rfc1867_orig_callback(event, event_data, extra TSRMLS_CC);
	}
	if (!PS(rfc1867_enabled)) {
		return retval;
	}

	progress = PS(rfc1867_progress);

	switch(event) {
		case MULTIPART_EVENT_START: {
			multipart_event_start *data = (multipart_event_start *) event_data;
			progress = ecalloc(1, sizeof(php_session_rfc1867_progress));
			progress->content_length = data->content_length;
			progress->sname_len  = strlen(PS(session_name));
			PS(rfc1867_progress) = progress;
		}
		break;
		case MULTIPART_EVENT_FORMDATA: {
			multipart_event_formdata *data = (multipart_event_formdata *) event_data;
			size_t value_len;

			if (Z_TYPE(progress->sid) && progress->key.c) {
				break;
			}

			/* orig callback may have modified *data->newlength */
			if (data->newlength) {
				value_len = *data->newlength;
			} else {
				value_len = data->length;
			}

			if (data->name && data->value && value_len) {
				size_t name_len = strlen(data->name);

				if (name_len == progress->sname_len && memcmp(data->name, PS(session_name), name_len) == 0) {
					zval_dtor(&progress->sid);
					ZVAL_STRINGL(&progress->sid, (*data->value), value_len, 1);

				} else if (name_len == PS(rfc1867_name).len && memcmp(data->name, PS(rfc1867_name).c, name_len) == 0) {
					smart_str_free(&progress->key);
					smart_str_appendl(&progress->key, PS(rfc1867_prefix).c, PS(rfc1867_prefix).len);
					smart_str_appendl(&progress->key, *data->value, value_len);
					smart_str_0(&progress->key);

					progress->apply_trans_sid = PS(use_trans_sid);
					php_session_rfc1867_early_find_sid(progress TSRMLS_CC);
				}
			}
		}
		break;
		case MULTIPART_EVENT_FILE_START: {
			multipart_event_file_start *data = (multipart_event_file_start *) event_data;

			/* Do nothing when $_POST["PHP_SESSION_UPLOAD_PROGRESS"] is not set 
			 * or when we have no session id */
			if (!Z_TYPE(progress->sid) || !progress->key.c) {
				break;
			}
			
			/* First FILE_START event, initializing data */
			if (!progress->data) {

				if (PS(rfc1867_freq) >= 0) {
					progress->update_step = PS(rfc1867_freq);
				} else if (PS(rfc1867_freq) < 0) { /* % of total size */
					progress->update_step = progress->content_length * -PS(rfc1867_freq) / 100;
				}
				progress->next_update = 0;
				progress->next_update_time = 0.0;

				ALLOC_INIT_ZVAL(progress->data);
				array_init(progress->data);

				ALLOC_INIT_ZVAL(progress->post_bytes_processed);
				ZVAL_LONG(progress->post_bytes_processed, data->post_bytes_processed);

				ALLOC_INIT_ZVAL(progress->files);
				array_init(progress->files);

				add_assoc_long_ex(progress->data, "start_time",      sizeof("start_time"),      (long)sapi_get_request_time(TSRMLS_C));
				add_assoc_long_ex(progress->data, "content_length",  sizeof("content_length"),  progress->content_length);
				add_assoc_zval_ex(progress->data, "bytes_processed", sizeof("bytes_processed"), progress->post_bytes_processed);
				add_assoc_bool_ex(progress->data, "done",            sizeof("done"),            0);
				add_assoc_zval_ex(progress->data, "files",           sizeof("files"),           progress->files);

				php_rinit_session(0 TSRMLS_CC);
				PS(id) = estrndup(Z_STRVAL(progress->sid), Z_STRLEN(progress->sid));
				PS(apply_trans_sid) = progress->apply_trans_sid;
				PS(send_cookie) = 0;
			}

			ALLOC_INIT_ZVAL(progress->current_file);
			array_init(progress->current_file);

			ALLOC_INIT_ZVAL(progress->current_file_bytes_processed);
			ZVAL_LONG(progress->current_file_bytes_processed, 0);

			/* Each uploaded file has its own array. Trying to make it close to $_FILES entries. */
			add_assoc_string_ex(progress->current_file, "field_name",    sizeof("field_name"),      data->name, 1);
			add_assoc_string_ex(progress->current_file, "name",          sizeof("name"),            *data->filename, 1);
			add_assoc_null_ex(progress->current_file, "tmp_name",        sizeof("tmp_name"));
			add_assoc_long_ex(progress->current_file, "error",           sizeof("error"),           0);

			add_assoc_bool_ex(progress->current_file, "done",            sizeof("done"),            0);
			add_assoc_long_ex(progress->current_file, "start_time",      sizeof("start_time"),      (long)time(NULL));
			add_assoc_zval_ex(progress->current_file, "bytes_processed", sizeof("bytes_processed"), progress->current_file_bytes_processed);

			add_next_index_zval(progress->files, progress->current_file);
			
			Z_LVAL_P(progress->post_bytes_processed) = data->post_bytes_processed;

			php_session_rfc1867_update(progress, 0 TSRMLS_CC);
		}
		break;
		case MULTIPART_EVENT_FILE_DATA: {
			multipart_event_file_data *data = (multipart_event_file_data *) event_data;

			if (!Z_TYPE(progress->sid) || !progress->key.c) {
				break;
			}
			
			Z_LVAL_P(progress->current_file_bytes_processed) = data->offset + data->length;
			Z_LVAL_P(progress->post_bytes_processed) = data->post_bytes_processed;

			php_session_rfc1867_update(progress, 0 TSRMLS_CC);
		}
		break;
		case MULTIPART_EVENT_FILE_END: {
			multipart_event_file_end *data = (multipart_event_file_end *) event_data;

			if (!Z_TYPE(progress->sid) || !progress->key.c) {
				break;
			}
			
			if (data->temp_filename) {
				add_assoc_string_ex(progress->current_file, "tmp_name",  sizeof("tmp_name"), data->temp_filename, 1);
			}
			add_assoc_long_ex(progress->current_file, "error", sizeof("error"), data->cancel_upload);
			add_assoc_bool_ex(progress->current_file, "done",  sizeof("done"),  1);

			Z_LVAL_P(progress->post_bytes_processed) = data->post_bytes_processed;

			php_session_rfc1867_update(progress, 0 TSRMLS_CC);
		}
		break;
		case MULTIPART_EVENT_END: {
			multipart_event_end *data = (multipart_event_end *) event_data;

			if (Z_TYPE(progress->sid) && progress->key.c) {
				if (PS(rfc1867_cleanup)) {
					php_session_rfc1867_cleanup(progress TSRMLS_CC);
				} else {
					add_assoc_bool_ex(progress->data, "done", sizeof("done"), 1);
					Z_LVAL_P(progress->post_bytes_processed) = data->post_bytes_processed;
					php_session_rfc1867_update(progress, 1 TSRMLS_CC);
				}
				php_rshutdown_session_globals(TSRMLS_C);
			}

			if (progress->data) {
				zval_ptr_dtor(&progress->data);
			}
			zval_dtor(&progress->sid);
			smart_str_free(&progress->key);
			efree(progress);
			progress = NULL;
			PS(rfc1867_progress) = NULL;
		}
		break;
	}

	if (progress && progress->cancel_upload) {
		return FAILURE;
	}
	return retval;

} /* }}} */