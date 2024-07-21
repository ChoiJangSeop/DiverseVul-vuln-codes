SAPI_POST_HANDLER_FUNC(php_mb_post_handler)
{
	const mbfl_encoding *detected;
	php_mb_encoding_handler_info_t info;

	MBSTRG(http_input_identify_post) = NULL;

	info.data_type              = PARSE_POST;
	info.separator              = "&";
	info.report_errors          = 0;
	info.to_encoding            = MBSTRG(internal_encoding);
	info.to_language            = MBSTRG(language);
	info.from_encodings         = MBSTRG(http_input_list);
	info.num_from_encodings     = MBSTRG(http_input_list_size); 
	info.from_language          = MBSTRG(language);

	detected = _php_mb_encoding_handler_ex(&info, arg, SG(request_info).post_data TSRMLS_CC);

	MBSTRG(http_input_identify) = detected;
	if (detected) {
		MBSTRG(http_input_identify_post) = detected;
	}
}