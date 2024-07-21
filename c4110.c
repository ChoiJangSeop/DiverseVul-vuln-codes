static size_t php_bz2iop_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC)
{
	struct php_bz2_stream_data_t *self = (struct php_bz2_stream_data_t *) stream->abstract;

	return BZ2_bzwrite(self->bz_file, (char*)buf, count); 
}