SAPI_API int sapi_add_header_ex(char *header_line, uint header_line_len, zend_bool duplicate, zend_bool replace TSRMLS_DC)
{
	sapi_header_line ctr = {0};
	int r;
	
	ctr.line = header_line;
	ctr.line_len = header_line_len;

	r = sapi_header_op(replace ? SAPI_HEADER_REPLACE : SAPI_HEADER_ADD,
			&ctr TSRMLS_CC);

	if (!duplicate)
		efree(header_line);

	return r;
}