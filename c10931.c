static void *php_libxml_streams_IO_open_wrapper(const char *filename, const char *mode, const int read_only)
{
	php_stream_statbuf ssbuf;
	php_stream_context *context = NULL;
	php_stream_wrapper *wrapper = NULL;
	char *resolved_path;
	const char *path_to_open = NULL;
	void *ret_val = NULL;
	int isescaped=0;
	xmlURI *uri;


	uri = xmlParseURI(filename);
	if (uri && (uri->scheme == NULL ||
			(xmlStrncmp(BAD_CAST uri->scheme, BAD_CAST "file", 4) == 0))) {
		resolved_path = xmlURIUnescapeString(filename, 0, NULL);
		isescaped = 1;
#if LIBXML_VERSION >= 20902 && defined(PHP_WIN32)
		/* Libxml 2.9.2 prefixes local paths with file:/ instead of file://,
			thus the php stream wrapper will fail on a valid case. For this
			reason the prefix is rather better cut off. */
		{
			size_t pre_len = sizeof("file:/") - 1;

			if (strncasecmp(resolved_path, "file:/", pre_len) == 0
				&& '/' != resolved_path[pre_len]) {
				xmlChar *tmp = xmlStrdup(resolved_path + pre_len);
				xmlFree(resolved_path);
				resolved_path = tmp;
			}
		}
#endif
	} else {
		resolved_path = (char *)filename;
	}

	if (uri) {
		xmlFreeURI(uri);
	}

	if (resolved_path == NULL) {
		return NULL;
	}

	/* logic copied from _php_stream_stat, but we only want to fail
	   if the wrapper supports stat, otherwise, figure it out from
	   the open.  This logic is only to support hiding warnings
	   that the streams layer puts out at times, but for libxml we
	   may try to open files that don't exist, but it is not a failure
	   in xml processing (eg. DTD files)  */
	wrapper = php_stream_locate_url_wrapper(resolved_path, &path_to_open, 0);
	if (wrapper && read_only && wrapper->wops->url_stat) {
		if (wrapper->wops->url_stat(wrapper, path_to_open, PHP_STREAM_URL_STAT_QUIET, &ssbuf, NULL) == -1) {
			if (isescaped) {
				xmlFree(resolved_path);
			}
			return NULL;
		}
	}

	context = php_stream_context_from_zval(Z_ISUNDEF(LIBXML(stream_context))? NULL : &LIBXML(stream_context), 0);

	ret_val = php_stream_open_wrapper_ex(path_to_open, (char *)mode, REPORT_ERRORS, NULL, context);
	if (ret_val) {
		/* Prevent from closing this by fclose() */
		((php_stream*)ret_val)->flags |= PHP_STREAM_FLAG_NO_FCLOSE;
	}
	if (isescaped) {
		xmlFree(resolved_path);
	}
	return ret_val;
}