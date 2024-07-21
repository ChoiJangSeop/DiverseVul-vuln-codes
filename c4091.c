SAPI_API void sapi_deactivate(TSRMLS_D)
{
	zend_llist_destroy(&SG(sapi_headers).headers);
	if (SG(request_info).post_data) {
		efree(SG(request_info).post_data);
	}  else 	if (SG(server_context)) {
		if(sapi_module.read_post) { 
			/* make sure we've consumed all request input data */
			char dummy[SAPI_POST_BLOCK_SIZE];
			int read_bytes;

			while((read_bytes = sapi_module.read_post(dummy, sizeof(dummy)-1 TSRMLS_CC)) > 0) {
				SG(read_post_bytes) += read_bytes;
			}
		}
	}
	if (SG(request_info).raw_post_data) {
		efree(SG(request_info).raw_post_data);
	} 
	if (SG(request_info).auth_user) {
		efree(SG(request_info).auth_user);
	}
	if (SG(request_info).auth_password) {
		efree(SG(request_info).auth_password);
	}
	if (SG(request_info).auth_digest) {
		efree(SG(request_info).auth_digest);
	}
	if (SG(request_info).content_type_dup) {
		efree(SG(request_info).content_type_dup);
	}
	if (SG(request_info).current_user) {
		efree(SG(request_info).current_user);
	}
	if (sapi_module.deactivate) {
		sapi_module.deactivate(TSRMLS_C);
	}
	if (SG(rfc1867_uploaded_files)) {
		destroy_uploaded_files_hash(TSRMLS_C);
	}
	if (SG(sapi_headers).mimetype) {
		efree(SG(sapi_headers).mimetype);
		SG(sapi_headers).mimetype = NULL;
	}
	sapi_send_headers_free(TSRMLS_C);
	SG(sapi_started) = 0;
	SG(headers_sent) = 0;
	SG(callback_run) = 0;
	if (SG(callback_func)) {
		zval_ptr_dtor(&SG(callback_func));
	}
	SG(request_info).headers_read = 0;
	SG(global_request_time) = 0;
}