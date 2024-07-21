mbfl_buffer_converter_feed_result(mbfl_buffer_converter *convd, mbfl_string *string, 
				  mbfl_string *result)
{
	if (convd == NULL || string == NULL || result == NULL) {
		return NULL;
	}
	mbfl_buffer_converter_feed(convd, string);
	if (convd->filter1 != NULL) {
		mbfl_convert_filter_flush(convd->filter1);
	}
	if (convd->filter2 != NULL) {
		mbfl_convert_filter_flush(convd->filter2);
	}
	result->no_encoding = convd->to->no_encoding;
	return mbfl_memory_device_result(&convd->device, result);
}