static int php_bz2iop_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	struct php_bz2_stream_data_t *self = (struct php_bz2_stream_data_t *)stream->abstract;
	int ret = EOF;
	
	if (close_handle) {
		BZ2_bzclose(self->bz_file);
	}

	if (self->stream) {
		php_stream_free(self->stream, PHP_STREAM_FREE_CLOSE | (close_handle == 0 ? PHP_STREAM_FREE_PRESERVE_HANDLE : 0));
	}

	efree(self);

	return ret;
}