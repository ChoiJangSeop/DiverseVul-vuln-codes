PS_READ_FUNC(files)
{
	long n;
	struct stat sbuf;
	PS_FILES_DATA;

	ps_files_open(data, key TSRMLS_CC);
	if (data->fd < 0) {
		return FAILURE;
	}

	if (fstat(data->fd, &sbuf)) {
		return FAILURE;
	}

	data->st_size = *vallen = sbuf.st_size;

	if (sbuf.st_size == 0) {
		*val = STR_EMPTY_ALLOC();
		return SUCCESS;
	}

	*val = emalloc(sbuf.st_size);

#if defined(HAVE_PREAD)
	n = pread(data->fd, *val, sbuf.st_size, 0);
#else
	lseek(data->fd, 0, SEEK_SET);
	n = read(data->fd, *val, sbuf.st_size);
#endif

	if (n != sbuf.st_size) {
		if (n == -1) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "read failed: %s (%d)", strerror(errno), errno);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "read returned less bytes than requested");
		}
		efree(*val);
		return FAILURE;
	}

	return SUCCESS;
}