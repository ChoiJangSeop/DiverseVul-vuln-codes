SAPI_API void sapi_activate(TSRMLS_D)
{
	zend_llist_init(&SG(sapi_headers).headers, sizeof(sapi_header_struct), (void (*)(void *)) sapi_free_header, 0);
	SG(sapi_headers).send_default_content_type = 1;

	/*
	SG(sapi_headers).http_response_code = 200;
	*/
	SG(sapi_headers).http_status_line = NULL;
	SG(sapi_headers).mimetype = NULL;
	SG(headers_sent) = 0;
	SG(callback_run) = 0;
	SG(callback_func) = NULL;
	SG(read_post_bytes) = 0;
	SG(request_info).post_data = NULL;
	SG(request_info).raw_post_data = NULL;
	SG(request_info).current_user = NULL;
	SG(request_info).current_user_length = 0;
	SG(request_info).no_headers = 0;
	SG(request_info).post_entry = NULL;
	SG(request_info).proto_num = 1000; /* Default to HTTP 1.0 */
	SG(global_request_time) = 0;

	/* It's possible to override this general case in the activate() callback, if necessary. */
	if (SG(request_info).request_method && !strcmp(SG(request_info).request_method, "HEAD")) {
		SG(request_info).headers_only = 1;
	} else {
		SG(request_info).headers_only = 0;
	}
	SG(rfc1867_uploaded_files) = NULL;

	/* Handle request method */
	if (SG(server_context)) {
		if (PG(enable_post_data_reading) && SG(request_info).request_method) {
			if (SG(request_info).content_type && !strcmp(SG(request_info).request_method, "POST")) {
				/* HTTP POST may contain form data to be processed into variables
				 * depending on given content type */
				sapi_read_post_data(TSRMLS_C);
			} else {
				/* Any other method with content payload will fill $HTTP_RAW_POST_DATA 
				 * if it is enabled by always_populate_raw_post_data. 
				 * It's up to the webserver to decide whether to allow a method or not. */
				SG(request_info).content_type_dup = NULL;
				if (sapi_module.default_post_reader) {
					sapi_module.default_post_reader(TSRMLS_C);
				}
			}
		} else {
			SG(request_info).content_type_dup = NULL;
		}

		/* Cookies */
		SG(request_info).cookie_data = sapi_module.read_cookies(TSRMLS_C);

		if (sapi_module.activate) {
			sapi_module.activate(TSRMLS_C);
		}
	}
	if (sapi_module.input_filter_init) {
		sapi_module.input_filter_init(TSRMLS_C);
	}
}