static void exif_error_docref(const char *docref EXIFERR_DC, const image_info_type *ImageInfo, int type, const char *format, ...)
{
	va_list args;
	
	va_start(args, format);
#ifdef EXIF_DEBUG
	{
		char *buf;

		spprintf(&buf, 0, "%s(%d): %s", _file, _line, format);
		php_verror(docref, ImageInfo->FileName?ImageInfo->FileName:"", type, buf, args TSRMLS_CC);
		efree(buf);
	}
#else
	php_verror(docref, ImageInfo->FileName?ImageInfo->FileName:"", type, format, args TSRMLS_CC);
#endif
	va_end(args);
}