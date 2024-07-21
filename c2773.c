PHP_FUNCTION(getcwd)
{
	char path[MAXPATHLEN];
	char *ret=NULL;
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

#if HAVE_GETCWD
	ret = VCWD_GETCWD(path, MAXPATHLEN);
#elif HAVE_GETWD
	ret = VCWD_GETWD(path);
#endif

	if (ret) {
		RETURN_STRING(path, 1);
	} else {
		RETURN_FALSE;
	}
}