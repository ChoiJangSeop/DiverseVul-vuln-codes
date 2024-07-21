SAPI_API SAPI_POST_READER_FUNC(sapi_read_standard_form_data)
{
	int read_bytes;
	int allocated_bytes=SAPI_POST_BLOCK_SIZE+1;

	if ((SG(post_max_size) > 0) && (SG(request_info).content_length > SG(post_max_size))) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "POST Content-Length of %ld bytes exceeds the limit of %ld bytes",
					SG(request_info).content_length, SG(post_max_size));
		return;
	}
	SG(request_info).post_data = emalloc(allocated_bytes);

	for (;;) {
		read_bytes = sapi_module.read_post(SG(request_info).post_data+SG(read_post_bytes), SAPI_POST_BLOCK_SIZE TSRMLS_CC);
		if (read_bytes<=0) {
			break;
		}
		SG(read_post_bytes) += read_bytes;
		if ((SG(post_max_size) > 0) && (SG(read_post_bytes) > SG(post_max_size))) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Actual POST length does not match Content-Length, and exceeds %ld bytes", SG(post_max_size));
			break;
		}
		if (read_bytes < SAPI_POST_BLOCK_SIZE) {
			break;
		}
		if (SG(read_post_bytes)+SAPI_POST_BLOCK_SIZE >= allocated_bytes) {
			allocated_bytes = SG(read_post_bytes)+SAPI_POST_BLOCK_SIZE+1;
			SG(request_info).post_data = erealloc(SG(request_info).post_data, allocated_bytes);
		}
	}
	SG(request_info).post_data[SG(read_post_bytes)] = 0;  /* terminating NULL */
	SG(request_info).post_data_length = SG(read_post_bytes);
}