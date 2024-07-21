static void _php_image_create_from(INTERNAL_FUNCTION_PARAMETERS, int image_type, char *tn, gdImagePtr (*func_p)(), gdImagePtr (*ioctx_func_p)())
{
	char *file;
	int file_len;
	long srcx, srcy, width, height;
	gdImagePtr im = NULL;
	php_stream *stream;
	FILE * fp = NULL;
	long ignore_warning;

	if (image_type == PHP_GDIMG_TYPE_GD2PART) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sllll", &file, &file_len, &srcx, &srcy, &width, &height) == FAILURE) {
			return;
		}
		if (width < 1 || height < 1) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Zero width or height not allowed");
			RETURN_FALSE;
		}
	} else {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &file, &file_len) == FAILURE) {
			return;
		}
	}


	stream = php_stream_open_wrapper(file, "rb", REPORT_ERRORS|IGNORE_PATH|IGNORE_URL_WIN, NULL);
	if (stream == NULL)	{
		RETURN_FALSE;
	}

	/* try and avoid allocating a FILE* if the stream is not naturally a FILE* */
	if (php_stream_is(stream, PHP_STREAM_IS_STDIO))	{
		if (FAILURE == php_stream_cast(stream, PHP_STREAM_AS_STDIO, (void**)&fp, REPORT_ERRORS)) {
			goto out_err;
		}
	} else if (ioctx_func_p) {
		/* we can create an io context */
		gdIOCtx* io_ctx;
		size_t buff_size;
		char *buff;

		/* needs to be malloc (persistent) - GD will free() it later */
		buff_size = php_stream_copy_to_mem(stream, &buff, PHP_STREAM_COPY_ALL, 1);

		if (!buff_size) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING,"Cannot read image data");
			goto out_err;
		}

		io_ctx = gdNewDynamicCtxEx(buff_size, buff, 0);
		if (!io_ctx) {
			pefree(buff, 1);
			php_error_docref(NULL TSRMLS_CC, E_WARNING,"Cannot allocate GD IO context");
			goto out_err;
		}

		if (image_type == PHP_GDIMG_TYPE_GD2PART) {
			im = (*ioctx_func_p)(io_ctx, srcx, srcy, width, height);
		} else {
			im = (*ioctx_func_p)(io_ctx);
		}
		io_ctx->gd_free(io_ctx);
		pefree(buff, 1);
	}
	else if (php_stream_can_cast(stream, PHP_STREAM_AS_STDIO)) {
		/* try and force the stream to be FILE* */
		if (FAILURE == php_stream_cast(stream, PHP_STREAM_AS_STDIO | PHP_STREAM_CAST_TRY_HARD, (void **) &fp, REPORT_ERRORS)) {
			goto out_err;
		}
	}

	if (!im && fp) {
		switch (image_type) {
			case PHP_GDIMG_TYPE_GD2PART:
				im = (*func_p)(fp, srcx, srcy, width, height);
				break;
#if defined(HAVE_GD_XPM)
			case PHP_GDIMG_TYPE_XPM:
				im = gdImageCreateFromXpm(file);
				break;
#endif

#ifdef HAVE_GD_JPG
			case PHP_GDIMG_TYPE_JPG:
				ignore_warning = INI_INT("gd.jpeg_ignore_warning");
				im = gdImageCreateFromJpegEx(fp, ignore_warning);
			break;
#endif

			default:
				im = (*func_p)(fp);
				break;
		}

		fflush(fp);
	}

/* register_im: */
	if (im) {
		ZEND_REGISTER_RESOURCE(return_value, im, le_gd);
		php_stream_close(stream);
		return;
	}

	php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid %s file", file, tn);
out_err:
	php_stream_close(stream);
	RETURN_FALSE;

}