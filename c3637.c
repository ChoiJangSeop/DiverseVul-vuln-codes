mbfl_ja_jp_hantozen(
    mbfl_string *string,
    mbfl_string *result,
    int mode)
{
	int n;
	unsigned char *p;
	const mbfl_encoding *encoding;
	mbfl_memory_device device;
	mbfl_convert_filter *decoder = NULL;
	mbfl_convert_filter *encoder = NULL;
	mbfl_convert_filter *tl_filter = NULL;
	mbfl_convert_filter *next_filter = NULL;
	mbfl_filt_tl_jisx0201_jisx0208_param *param = NULL;

	/* validate parameters */
	if (string == NULL || result == NULL) {
		return NULL;
	}

	encoding = mbfl_no2encoding(string->no_encoding);
	if (encoding == NULL) {
		return NULL;
	}

	mbfl_memory_device_init(&device, string->len, 0);
	mbfl_string_init(result);

	result->no_language = string->no_language;
	result->no_encoding = string->no_encoding;

	decoder = mbfl_convert_filter_new(
		mbfl_no_encoding_wchar,
		string->no_encoding,
		mbfl_memory_device_output, 0, &device);
	if (decoder == NULL) {
		goto out;
	}
	next_filter = decoder;

	param =
		(mbfl_filt_tl_jisx0201_jisx0208_param *)mbfl_malloc(sizeof(mbfl_filt_tl_jisx0201_jisx0208_param));
	if (param == NULL) {
		goto out;
	}

	param->mode = mode;

	tl_filter = mbfl_convert_filter_new2(
		&vtbl_tl_jisx0201_jisx0208,
		(int(*)(int, void*))next_filter->filter_function,
		(int(*)(void*))next_filter->filter_flush,
		next_filter);
	if (tl_filter == NULL) {
		mbfl_free(param);
		goto out;
	}

	tl_filter->opaque = param;
	next_filter = tl_filter;

	encoder = mbfl_convert_filter_new(
		string->no_encoding,
		mbfl_no_encoding_wchar,
		(int(*)(int, void*))next_filter->filter_function,
		(int(*)(void*))next_filter->filter_flush,
		next_filter);	
	if (encoder == NULL) {
		goto out;
	}

	/* feed data */
	p = string->val;
	n = string->len;
	if (p != NULL) {
		while (n > 0) {
			if ((*encoder->filter_function)(*p++, encoder) < 0) {
				break;
			}
			n--;
		}
	}

	mbfl_convert_filter_flush(encoder);
	result = mbfl_memory_device_result(&device, result);
out:
	if (tl_filter != NULL) {
		if (tl_filter->opaque != NULL) {
			mbfl_free(tl_filter->opaque);
		}
		mbfl_convert_filter_delete(tl_filter);
	}

	if (decoder != NULL) {
		mbfl_convert_filter_delete(decoder);
	}

	if (encoder != NULL) {
		mbfl_convert_filter_delete(encoder);
	}

	return result;
}