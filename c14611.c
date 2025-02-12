PHP_FUNCTION(linkinfo)
{
	char *link;
	size_t link_len;
	zend_stat_t sb;
	int ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "p", &link, &link_len) == FAILURE) {
		return;
	}

	ret = VCWD_STAT(link, &sb);
	if (ret == -1) {
		php_error_docref(NULL, E_WARNING, "%s", strerror(errno));
		RETURN_LONG(Z_L(-1));
	}

	RETURN_LONG((zend_long) sb.st_dev);
}