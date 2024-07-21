SAPI_API void sapi_activate_headers_only(TSRMLS_D)
{
	if (SG(request_info).headers_read == 1)
		return;
	SG(request_info).headers_read = 1;
	zend_llist_init(&SG(sapi_headers).headers, sizeof(sapi_header_struct), 
			(void (*)(void *)) sapi_free_header, 0);
	SG(sapi_headers).send_default_content_type = 1;

	/* SG(sapi_headers).http_response_code = 200; */ 
	SG(sapi_headers).http_status_line = NULL;
	SG(sapi_headers).mimetype = NULL;
	SG(read_post_bytes) = 0;
	SG(request_info).post_data = NULL;
	SG(request_info).raw_post_data = NULL;
	SG(request_info).current_user = NULL;
	SG(request_info).current_user_length = 0;
	SG(request_info).no_headers = 0;
	SG(request_info).post_entry = NULL;
	SG(global_request_time) = 0;

	/*
	 * It's possible to override this general case in the activate() callback, 
	 * if necessary.
	 */
	if (SG(request_info).request_method && !strcmp(SG(request_info).request_method, "HEAD")) {
		SG(request_info).headers_only = 1;
	} else {
		SG(request_info).headers_only = 0;
	}
	if (SG(server_context)) {
		SG(request_info).cookie_data = sapi_module.read_cookies(TSRMLS_C);
		if (sapi_module.activate) {
			sapi_module.activate(TSRMLS_C);
		}
	}
	if (sapi_module.input_filter_init ) {
		sapi_module.input_filter_init(TSRMLS_C);
	}
}