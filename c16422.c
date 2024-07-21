static int php_stream_input_close(php_stream *stream, int close_handle TSRMLS_DC) /* {{{ */
{
	efree(stream->abstract);

	return 0;
}