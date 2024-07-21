PHP_BZ2_API php_stream *_php_stream_bz2open_from_BZFILE(BZFILE *bz, 
														char *mode, php_stream *innerstream STREAMS_DC TSRMLS_DC)
{
	struct php_bz2_stream_data_t *self;
	
	self = emalloc(sizeof(*self));

	self->stream = innerstream;
	self->bz_file = bz;

	return php_stream_alloc_rel(&php_stream_bz2io_ops, self, 0, mode);
}