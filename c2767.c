PHP_FUNCTION(getenv)
{
	char *ptr, *str;
	int str_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
		RETURN_FALSE;
	}

	/* SAPI method returns an emalloc()'d string */
	ptr = sapi_getenv(str, str_len TSRMLS_CC);
	if (ptr) {
		RETURN_STRING(ptr, 0);
	}
#ifdef PHP_WIN32
	{
		char dummybuf;
		int size;

		SetLastError(0);
		/*If the given bugger is not large enough to hold the data, the return value is 
		the buffer size,  in characters, required to hold the string and its terminating 
		null character. We use this return value to alloc the final buffer. */
		size = GetEnvironmentVariableA(str, &dummybuf, 0);
		if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
				/* The environment variable doesn't exist. */
				RETURN_FALSE;
		}

		if (size == 0) {
				/* env exists, but it is empty */
				RETURN_EMPTY_STRING();
		}

		ptr = emalloc(size);
		size = GetEnvironmentVariableA(str, ptr, size);
		if (size == 0) {
				/* has been removed between the two calls */
				efree(ptr);
				RETURN_EMPTY_STRING();
		} else {
			RETURN_STRING(ptr, 0);
		}
	}
#else
	/* system method returns a const */
	ptr = getenv(str);
	if (ptr) {
		RETURN_STRING(ptr, 1);
	}
#endif
	RETURN_FALSE;
}