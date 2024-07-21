static int exif_read_from_impl(image_info_type *ImageInfo, php_stream *stream, int read_thumbnail, int read_all)
{
	int ret;
	zend_stat_t st;

	/* Start with an empty image information structure. */
	memset(ImageInfo, 0, sizeof(*ImageInfo));

	ImageInfo->motorola_intel	= -1; /* flag as unknown */
	ImageInfo->infile			= stream;
	ImageInfo->FileName			= NULL;

	if (php_stream_is(ImageInfo->infile, PHP_STREAM_IS_STDIO)) {
		if (VCWD_STAT(stream->orig_path, &st) >= 0) {
			zend_string *base;
			if ((st.st_mode & S_IFMT) != S_IFREG) {
				exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_WARNING, "Not a file");
				php_stream_close(ImageInfo->infile);
				return FALSE;
			}

			/* Store file name */
			base = php_basename(stream->orig_path, strlen(stream->orig_path), NULL, 0);
			ImageInfo->FileName = estrndup(ZSTR_VAL(base), ZSTR_LEN(base));

			zend_string_release(base);

			/* Store file date/time. */
			ImageInfo->FileDateTime = st.st_mtime;
			ImageInfo->FileSize = st.st_size;
			/*exif_error_docref(NULL EXIFERR_CC, ImageInfo, E_NOTICE, "Opened stream is file: %d", ImageInfo->FileSize);*/
		}
	} else {
		if (!ImageInfo->FileSize) {
			php_stream_seek(ImageInfo->infile, 0, SEEK_END);
			ImageInfo->FileSize = php_stream_tell(ImageInfo->infile);
			php_stream_seek(ImageInfo->infile, 0, SEEK_SET);
		}
	}

	ImageInfo->read_thumbnail		= read_thumbnail;
	ImageInfo->read_all				= read_all;
	ImageInfo->Thumbnail.filetype	= IMAGE_FILETYPE_UNKNOWN;

	ImageInfo->encode_unicode		= estrdup(EXIF_G(encode_unicode));
	ImageInfo->decode_unicode_be	= estrdup(EXIF_G(decode_unicode_be));
	ImageInfo->decode_unicode_le	= estrdup(EXIF_G(decode_unicode_le));
	ImageInfo->encode_jis			= estrdup(EXIF_G(encode_jis));
	ImageInfo->decode_jis_be	 	= estrdup(EXIF_G(decode_jis_be));
	ImageInfo->decode_jis_le		= estrdup(EXIF_G(decode_jis_le));


	ImageInfo->ifd_nesting_level = 0;

	/* Scan the headers */
	ret = exif_scan_FILE_header(ImageInfo);

	return ret;
}