static size_t php_stream_input_read(php_stream *stream, char *buf, size_t count TSRMLS_DC) /* {{{ */
{
	off_t *position = (off_t*)stream->abstract;
	size_t read_bytes = 0;

	if (!stream->eof) {
		if (SG(request_info).raw_post_data) { /* data has already been read by a post handler */
			read_bytes = SG(request_info).raw_post_data_length - *position;
			if (read_bytes <= count) {
				stream->eof = 1;
			} else {
				read_bytes = count;
			}
			if (read_bytes) {
				memcpy(buf, SG(request_info).raw_post_data + *position, read_bytes);
			}
		} else if (sapi_module.read_post) {
			read_bytes = sapi_module.read_post(buf, count TSRMLS_CC);
			if (read_bytes <= 0) {
				stream->eof = 1;
				read_bytes = 0;
			}
			/* Increment SG(read_post_bytes) only when something was actually read. */
			SG(read_post_bytes) += read_bytes;
		} else {
			stream->eof = 1;
		}
	}

	*position += read_bytes;

	return read_bytes;
}