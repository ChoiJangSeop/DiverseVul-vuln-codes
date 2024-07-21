php_libxml_input_buffer_create_filename(const char *URI, xmlCharEncoding enc)
{
	xmlParserInputBufferPtr ret;
	void *context = NULL;

	if (URI == NULL)
		return(NULL);

	context = php_libxml_streams_IO_open_read_wrapper(URI);

	if (context == NULL) {
		return(NULL);
	}

	/* Allocate the Input buffer front-end. */
	ret = xmlAllocParserInputBuffer(enc);
	if (ret != NULL) {
		ret->context = context;
		ret->readcallback = php_libxml_streams_IO_read;
		ret->closecallback = php_libxml_streams_IO_close;
	} else
		php_libxml_streams_IO_close(context);

	return(ret);
}