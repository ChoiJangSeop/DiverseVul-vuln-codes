PHP_BZ2_API php_stream *_php_stream_bz2open(php_stream_wrapper *wrapper,
											char *path,
											char *mode,
											int options,
											char **opened_path,
											php_stream_context *context STREAMS_DC TSRMLS_DC)
{
	php_stream *retstream = NULL, *stream = NULL;
	char *path_copy = NULL;
	BZFILE *bz_file = NULL;

	if (strncasecmp("compress.bzip2://", path, 17) == 0) {
		path += 17;
	}
	if (mode[0] == '\0' || (mode[0] != 'w' && mode[0] != 'r' && mode[1] != '\0')) {
		return NULL;
	}

#ifdef VIRTUAL_DIR
	virtual_filepath_ex(path, &path_copy, NULL TSRMLS_CC);
#else
	path_copy = path;
#endif  

	if (php_check_open_basedir(path_copy TSRMLS_CC)) {
#ifdef VIRTUAL_DIR
		free(path_copy);
#endif
		return NULL;
	}
	
	/* try and open it directly first */
	bz_file = BZ2_bzopen(path_copy, mode);

	if (opened_path && bz_file) {
		*opened_path = estrdup(path_copy);
	}
#ifdef VIRTUAL_DIR
	free(path_copy);
#endif
	path_copy = NULL;
	
	if (bz_file == NULL) {
		/* that didn't work, so try and get something from the network/wrapper */
		stream = php_stream_open_wrapper(path, mode, options | STREAM_WILL_CAST, opened_path);
	
		if (stream) {
			int fd;
			if (SUCCESS == php_stream_cast(stream, PHP_STREAM_AS_FD, (void **) &fd, REPORT_ERRORS)) {
				bz_file = BZ2_bzdopen(fd, mode);
			}
		}

		/* remove the file created by php_stream_open_wrapper(), it is not needed since BZ2 functions
		 * failed.
		 */
		if (opened_path && !bz_file && mode[0] == 'w') {
			VCWD_UNLINK(*opened_path);
		}
	}
	
	if (bz_file) {
		retstream = _php_stream_bz2open_from_BZFILE(bz_file, mode, stream STREAMS_REL_CC TSRMLS_CC);
		if (retstream) {
			return retstream;
		}

		BZ2_bzclose(bz_file);
	}

	if (stream) {
		php_stream_close(stream);
	}

	return NULL;
}