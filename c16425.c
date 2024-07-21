SAPI_API void sapi_handle_post(void *arg TSRMLS_DC)
{
	if (SG(request_info).post_entry && SG(request_info).content_type_dup) {
		SG(request_info).post_entry->post_handler(SG(request_info).content_type_dup, arg TSRMLS_CC);
		if (SG(request_info).post_data) {
			efree(SG(request_info).post_data);
			SG(request_info).post_data = NULL;
		}
		efree(SG(request_info).content_type_dup);
		SG(request_info).content_type_dup = NULL;
	}
}