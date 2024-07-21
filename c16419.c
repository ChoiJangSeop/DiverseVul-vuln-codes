php_stream * php_stream_url_wrap_php(php_stream_wrapper *wrapper, const char *path, const char *mode, int options,
									 char **opened_path, php_stream_context *context STREAMS_DC TSRMLS_DC) /* {{{ */
{
	int fd = -1;
	int mode_rw = 0;
	php_stream * stream = NULL;
	char *p, *token, *pathdup;
	long max_memory;
	FILE *file = NULL;

	if (!strncasecmp(path, "php://", 6)) {
		path += 6;
	}

	if (!strncasecmp(path, "temp", 4)) {
		path += 4;
		max_memory = PHP_STREAM_MAX_MEM;
		if (!strncasecmp(path, "/maxmemory:", 11)) {
			path += 11;
			max_memory = strtol(path, NULL, 10);
			if (max_memory < 0) {
				php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "Max memory must be >= 0");
				return NULL;
			}
		}
		if (strpbrk(mode, "wa+")) {
			mode_rw = TEMP_STREAM_DEFAULT;
		} else {
			mode_rw = TEMP_STREAM_READONLY;
		}
		return php_stream_temp_create(mode_rw, max_memory);
	}

	if (!strcasecmp(path, "memory")) {
		if (strpbrk(mode, "wa+")) {
			mode_rw = TEMP_STREAM_DEFAULT;
		} else {
			mode_rw = TEMP_STREAM_READONLY;
		}
		return php_stream_memory_create(mode_rw);
	}

	if (!strcasecmp(path, "output")) {
		return php_stream_alloc(&php_stream_output_ops, NULL, 0, "wb");
	}

	if (!strcasecmp(path, "input")) {
		if ((options & STREAM_OPEN_FOR_INCLUDE) && !PG(allow_url_include) ) {
			if (options & REPORT_ERRORS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "URL file-access is disabled in the server configuration");
			}
			return NULL;
		}
		return php_stream_alloc(&php_stream_input_ops, ecalloc(1, sizeof(off_t)), 0, "rb");
	}

	if (!strcasecmp(path, "stdin")) {
		if ((options & STREAM_OPEN_FOR_INCLUDE) && !PG(allow_url_include) ) {
			if (options & REPORT_ERRORS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "URL file-access is disabled in the server configuration");
			}
			return NULL;
		}
		if (!strcmp(sapi_module.name, "cli")) {
			static int cli_in = 0;
			fd = STDIN_FILENO;
			if (cli_in) {
				fd = dup(fd);
			} else {
				cli_in = 1;
				file = stdin;
			}
		} else {
			fd = dup(STDIN_FILENO);
		}
	} else if (!strcasecmp(path, "stdout")) {
		if (!strcmp(sapi_module.name, "cli")) {
			static int cli_out = 0;
			fd = STDOUT_FILENO;
			if (cli_out++) {
				fd = dup(fd);
			} else {
				cli_out = 1;
				file = stdout;
			}
		} else {
			fd = dup(STDOUT_FILENO);
		}
	} else if (!strcasecmp(path, "stderr")) {
		if (!strcmp(sapi_module.name, "cli")) {
			static int cli_err = 0;
			fd = STDERR_FILENO;
			if (cli_err++) {
				fd = dup(fd);
			} else {
				cli_err = 1;
				file = stderr;
			}
		} else {
			fd = dup(STDERR_FILENO);
		}
	} else if (!strncasecmp(path, "fd/", 3)) {
		char	   *start,
				   *end;
		long	   fildes_ori;
		int		   dtablesize;

		if (strcmp(sapi_module.name, "cli")) {
			if (options & REPORT_ERRORS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Direct access to file descriptors is only available from command-line PHP");
			}
			return NULL;
		}

		if ((options & STREAM_OPEN_FOR_INCLUDE) && !PG(allow_url_include) ) {
			if (options & REPORT_ERRORS) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "URL file-access is disabled in the server configuration");
			}
			return NULL;
		}

		start = &path[3];
		fildes_ori = strtol(start, &end, 10);
		if (end == start || *end != '\0') {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC,
				"php://fd/ stream must be specified in the form php://fd/<orig fd>");
			return NULL;
		}

#if HAVE_UNISTD_H
		dtablesize = getdtablesize();
#else
		dtablesize = INT_MAX;
#endif

		if (fildes_ori < 0 || fildes_ori >= dtablesize) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC,
				"The file descriptors must be non-negative numbers smaller than %d", dtablesize);
			return NULL;
		}
		
		fd = dup(fildes_ori);
		if (fd == -1) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC,
				"Error duping file descriptor %ld; possibly it doesn't exist: "
				"[%d]: %s", fildes_ori, errno, strerror(errno));
			return NULL;
		}
	} else if (!strncasecmp(path, "filter/", 7)) {
		/* Save time/memory when chain isn't specified */
		if (strchr(mode, 'r') || strchr(mode, '+')) {
			mode_rw |= PHP_STREAM_FILTER_READ;
		}
		if (strchr(mode, 'w') || strchr(mode, '+') || strchr(mode, 'a')) {
			mode_rw |= PHP_STREAM_FILTER_WRITE;
		}
		pathdup = estrndup(path + 6, strlen(path + 6));
		p = strstr(pathdup, "/resource=");
		if (!p) {
			php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "No URL resource specified");
			efree(pathdup);
			return NULL;
		}
		if (!(stream = php_stream_open_wrapper(p + 10, mode, options, opened_path))) {
			efree(pathdup);
			return NULL;
		}

		*p = '\0';

		p = php_strtok_r(pathdup + 1, "/", &token);
		while (p) {
			if (!strncasecmp(p, "read=", 5)) {
				php_stream_apply_filter_list(stream, p + 5, 1, 0 TSRMLS_CC);
			} else if (!strncasecmp(p, "write=", 6)) {
				php_stream_apply_filter_list(stream, p + 6, 0, 1 TSRMLS_CC);
			} else {
				php_stream_apply_filter_list(stream, p, mode_rw & PHP_STREAM_FILTER_READ, mode_rw & PHP_STREAM_FILTER_WRITE TSRMLS_CC);
			}
			p = php_strtok_r(NULL, "/", &token);
		}
		efree(pathdup);

		return stream;
	} else {
		/* invalid php://thingy */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid php:// URL specified");
		return NULL;
	}

	/* must be stdin, stderr or stdout */
	if (fd == -1)	{
		/* failed to dup */
		return NULL;
	}

#if defined(S_IFSOCK) && !defined(WIN32) && !defined(__BEOS__)
	do {
		struct stat st;
		memset(&st, 0, sizeof(st));
		if (fstat(fd, &st) == 0 && (st.st_mode & S_IFMT) == S_IFSOCK) {
			stream = php_stream_sock_open_from_socket(fd, NULL);
			if (stream) {
				stream->ops = &php_stream_socket_ops;
				return stream;
			}
		}
	} while (0);
#endif

	if (file) {
		stream = php_stream_fopen_from_file(file, mode);
	} else {
		stream = php_stream_fopen_from_fd(fd, mode, NULL);
		if (stream == NULL) {
			close(fd);
		}
	}

	return stream;
}