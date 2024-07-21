SAPI_API SAPI_POST_READER_FUNC(php_default_post_reader)
{
	char *data;
	int length;

	/* $HTTP_RAW_POST_DATA registration */
	if (!strcmp(SG(request_info).request_method, "POST")) {
		if (NULL == SG(request_info).post_entry) {
			/* no post handler registered, so we just swallow the data */
			sapi_read_standard_form_data(TSRMLS_C);
		}

		/* For unknown content types we create HTTP_RAW_POST_DATA even if always_populate_raw_post_data off,
		 * this is in-effecient, but we need to keep doing it for BC reasons (for now) */
		if ((PG(always_populate_raw_post_data) || NULL == SG(request_info).post_entry) && SG(request_info).post_data) {
			length = SG(request_info).post_data_length;
			data = estrndup(SG(request_info).post_data, length);
			SET_VAR_STRINGL("HTTP_RAW_POST_DATA", data, length);
		}
	}

	/* for php://input stream:
	 some post handlers modify the content of request_info.post_data
	 so for now we need a copy for the php://input stream
	 in the long run post handlers should be changed to not touch
	 request_info.post_data for memory preservation reasons
	*/
	if (SG(request_info).post_data) {
		SG(request_info).raw_post_data = estrndup(SG(request_info).post_data, SG(request_info).post_data_length);
		SG(request_info).raw_post_data_length = SG(request_info).post_data_length;
	}
}