PHP_FUNCTION(exif_thumbnail)
{
	int ret, arg_c = ZEND_NUM_ARGS();
	image_info_type ImageInfo;
	zval *stream;
	zval *z_width = NULL, *z_height = NULL, *z_imagetype = NULL;

	/* Parse arguments */
	ZEND_PARSE_PARAMETERS_START(1, 4)
		Z_PARAM_ZVAL(stream)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(z_width)
		Z_PARAM_ZVAL(z_height)
		Z_PARAM_ZVAL(z_imagetype)
	ZEND_PARSE_PARAMETERS_END();

	memset(&ImageInfo, 0, sizeof(ImageInfo));

	if (Z_TYPE_P(stream) == IS_RESOURCE) {
		php_stream *p_stream = NULL;

		php_stream_from_res(p_stream, Z_RES_P(stream));

		ret = exif_read_from_stream(&ImageInfo, p_stream, 1, 0);
	} else {
		convert_to_string(stream);

		if (!Z_STRLEN_P(stream)) {
			exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_WARNING, "Filename cannot be empty");

			RETURN_FALSE;
		}

		ret = exif_read_from_file(&ImageInfo, Z_STRVAL_P(stream), 1, 0);
	}

	if (ret == FALSE) {
		exif_discard_imageinfo(&ImageInfo);
		RETURN_FALSE;
	}

#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Thumbnail data %d %d %d, %d x %d", ImageInfo.Thumbnail.data, ImageInfo.Thumbnail.size, ImageInfo.Thumbnail.filetype, ImageInfo.Thumbnail.width, ImageInfo.Thumbnail.height);
#endif
	if (!ImageInfo.Thumbnail.data || !ImageInfo.Thumbnail.size) {
		exif_discard_imageinfo(&ImageInfo);
		RETURN_FALSE;
	}

#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Returning thumbnail(%d)", ImageInfo.Thumbnail.size);
#endif

	ZVAL_STRINGL(return_value, ImageInfo.Thumbnail.data, ImageInfo.Thumbnail.size);
	if (arg_c >= 3) {
		if (!ImageInfo.Thumbnail.width || !ImageInfo.Thumbnail.height) {
			exif_scan_thumbnail(&ImageInfo);
		}
		ZEND_TRY_ASSIGN_LONG(z_width,  ImageInfo.Thumbnail.width);
		ZEND_TRY_ASSIGN_LONG(z_height, ImageInfo.Thumbnail.height);
	}
	if (arg_c >= 4)	{
		ZEND_TRY_ASSIGN_LONG(z_imagetype, ImageInfo.Thumbnail.filetype);
	}

#ifdef EXIF_DEBUG
	exif_error_docref(NULL EXIFERR_CC, &ImageInfo, E_NOTICE, "Discarding info");
#endif

	exif_discard_imageinfo(&ImageInfo);

#ifdef EXIF_DEBUG
	php_error_docref1(NULL, (Z_TYPE_P(stream) == IS_RESOURCE ? "<stream>" : Z_STRVAL_P(stream)), E_NOTICE, "Done");
#endif
}