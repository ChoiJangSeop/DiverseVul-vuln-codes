mbfl_strcut(
    mbfl_string *string,
    mbfl_string *result,
    int from,
    int length)
{
	const mbfl_encoding *encoding;
	mbfl_memory_device device;

	/* validate the parameters */
	if (string == NULL || string->val == NULL || result == NULL) {
		return NULL;
	}

	if (from < 0 || length < 0) {
		return NULL;
	}

	if (from >= string->len) {
		from = string->len;
	}

	encoding = mbfl_no2encoding(string->no_encoding);
	if (encoding == NULL) {
		return NULL;
	}

	mbfl_string_init(result);
	result->no_language = string->no_language;
	result->no_encoding = string->no_encoding;

	if ((encoding->flag & (MBFL_ENCTYPE_SBCS
				| MBFL_ENCTYPE_WCS2BE
				| MBFL_ENCTYPE_WCS2LE
				| MBFL_ENCTYPE_WCS4BE
				| MBFL_ENCTYPE_WCS4LE))
			|| encoding->mblen_table != NULL) {
		const unsigned char *start = NULL;
		const unsigned char *end = NULL;
		unsigned char *w;
		unsigned int sz;

		if (encoding->flag & (MBFL_ENCTYPE_WCS2BE | MBFL_ENCTYPE_WCS2LE)) {
			from &= -2;

			if (from + length >= string->len) {
				length = string->len - from;
			}

			start = string->val + from;
			end   = start + (length & -2);
		} else if (encoding->flag & (MBFL_ENCTYPE_WCS4BE | MBFL_ENCTYPE_WCS4LE)) {
			from &= -4;

			if (from + length >= string->len) {
				length = string->len - from;
			}

			start = string->val + from;
			end   = start + (length & -4);
		} else if ((encoding->flag & MBFL_ENCTYPE_SBCS)) {
			if (from + length >= string->len) {
				length = string->len - from;
			}

			start = string->val + from;
			end = start + length;
		} else if (encoding->mblen_table != NULL) {
			const unsigned char *mbtab = encoding->mblen_table;
			const unsigned char *p, *q;
			int m;

			/* search start position */
			for (m = 0, p = string->val, q = p + from;
					p < q; p += (m = mbtab[*p]));

			if (p > q) {
				p -= m;
			}

			start = p;

			/* search end position */
			if ((start - string->val) + length >= (int)string->len) {
				end = string->val + string->len;
			} else {
				for (q = p + length; p < q; p += (m = mbtab[*p]));

				if (p > q) {
					p -= m;
				}
				end = p;
			}
		} else {
			/* never reached */
			return NULL;
		}

		/* allocate memory and copy string */
		sz = end - start;
		if ((w = (unsigned char*)mbfl_calloc(sz + 8,
				sizeof(unsigned char))) == NULL) {
			return NULL;
		}

		memcpy(w, start, sz);
		w[sz] = '\0';
		w[sz + 1] = '\0';
		w[sz + 2] = '\0';
		w[sz + 3] = '\0';

		result->val = w;
		result->len = sz;
	} else {
		mbfl_convert_filter *encoder     = NULL;
		mbfl_convert_filter *decoder     = NULL;
		const unsigned char *p, *q, *r;
		struct {
			mbfl_convert_filter encoder;
			mbfl_convert_filter decoder;
			const unsigned char *p;
			int pos;
		} bk, _bk;

		/* output code filter */
		if (!(decoder = mbfl_convert_filter_new(
				mbfl_no_encoding_wchar,
				string->no_encoding,
				mbfl_memory_device_output, 0, &device))) {
			return NULL;
		}

		/* wchar filter */
		if (!(encoder = mbfl_convert_filter_new(
				string->no_encoding,
				mbfl_no_encoding_wchar,
				mbfl_filter_output_null,
				NULL, NULL))) {
			mbfl_convert_filter_delete(decoder);
			return NULL;
		}

		mbfl_memory_device_init(&device, length + 8, 0);

		p = string->val;

		/* search start position */
		for (q = string->val + from; p < q; p++) {
			(*encoder->filter_function)(*p, encoder);
		}

		/* switch the drain direction */
		encoder->output_function = (int(*)(int,void *))decoder->filter_function;
		encoder->flush_function = (int(*)(void *))decoder->filter_flush;
		encoder->data = decoder;

		q = string->val + string->len;

		/* save the encoder, decoder state and the pointer */
		mbfl_convert_filter_copy(decoder, &_bk.decoder);
		mbfl_convert_filter_copy(encoder, &_bk.encoder);
		_bk.p = p;
		_bk.pos = device.pos;

		if (length > q - p) {
			length = q - p;
		}

		if (length >= 20) {
			/* output a little shorter than "length" */
			/* XXX: the constant "20" was determined purely on the heuristics. */
			for (r = p + length - 20; p < r; p++) {
				(*encoder->filter_function)(*p, encoder);
			}

			/* if the offset of the resulting string exceeds the length,
			 * then restore the state */
			if (device.pos > length) {
				p = _bk.p;
				device.pos = _bk.pos;
				decoder->filter_dtor(decoder);
				encoder->filter_dtor(encoder);
				mbfl_convert_filter_copy(&_bk.decoder, decoder);
				mbfl_convert_filter_copy(&_bk.encoder, encoder);
				bk = _bk;
			} else {
				/* save the encoder, decoder state and the pointer */
				mbfl_convert_filter_copy(decoder, &bk.decoder);
				mbfl_convert_filter_copy(encoder, &bk.encoder);
				bk.p = p;
				bk.pos = device.pos;

				/* flush the stream */
				(*encoder->filter_flush)(encoder);

				/* if the offset of the resulting string exceeds the length,
				 * then restore the state */
				if (device.pos > length) {
					bk.decoder.filter_dtor(&bk.decoder);
					bk.encoder.filter_dtor(&bk.encoder);

					p = _bk.p;
					device.pos = _bk.pos;
					decoder->filter_dtor(decoder);
					encoder->filter_dtor(encoder);
					mbfl_convert_filter_copy(&_bk.decoder, decoder);
					mbfl_convert_filter_copy(&_bk.encoder, encoder);
					bk = _bk;
				} else {
					_bk.decoder.filter_dtor(&_bk.decoder);
					_bk.encoder.filter_dtor(&_bk.encoder);

					p = bk.p;
					device.pos = bk.pos;
					decoder->filter_dtor(decoder);
					encoder->filter_dtor(encoder);
					mbfl_convert_filter_copy(&bk.decoder, decoder);
					mbfl_convert_filter_copy(&bk.encoder, encoder);
				}
			}
		} else {
			bk = _bk;
		}

		/* detect end position */
		while (p < q) {
			(*encoder->filter_function)(*p, encoder);

			if (device.pos > length) {
				/* restore filter */
				p = bk.p;
				device.pos = bk.pos;
				decoder->filter_dtor(decoder);
				encoder->filter_dtor(encoder);
				mbfl_convert_filter_copy(&bk.decoder, decoder);
				mbfl_convert_filter_copy(&bk.encoder, encoder);
				break;
			}

			p++;

			/* backup current state */
			mbfl_convert_filter_copy(decoder, &_bk.decoder);
			mbfl_convert_filter_copy(encoder, &_bk.encoder);
			_bk.pos = device.pos;
			_bk.p = p;

			(*encoder->filter_flush)(encoder);

			if (device.pos > length) {
				_bk.decoder.filter_dtor(&_bk.decoder);
				_bk.encoder.filter_dtor(&_bk.encoder);

				/* restore filter */
				p = bk.p;
				device.pos = bk.pos;
				decoder->filter_dtor(decoder);
				encoder->filter_dtor(encoder);
				mbfl_convert_filter_copy(&bk.decoder, decoder);
				mbfl_convert_filter_copy(&bk.encoder, encoder);
				break;
			}

			bk.decoder.filter_dtor(&bk.decoder);
			bk.encoder.filter_dtor(&bk.encoder);

			p = _bk.p;
			device.pos = _bk.pos;
			decoder->filter_dtor(decoder);
			encoder->filter_dtor(encoder);
			mbfl_convert_filter_copy(&_bk.decoder, decoder);
			mbfl_convert_filter_copy(&_bk.encoder, encoder);

			bk = _bk;
		}

		(*encoder->filter_flush)(encoder);

		bk.decoder.filter_dtor(&bk.decoder);
		bk.encoder.filter_dtor(&bk.encoder);

		result = mbfl_memory_device_result(&device, result);

		mbfl_convert_filter_delete(encoder);
		mbfl_convert_filter_delete(decoder);
	}

	return result;
}