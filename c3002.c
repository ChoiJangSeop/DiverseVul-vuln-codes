PHPAPI char *php_session_create_id(PS_CREATE_SID_ARGS) /* {{{ */
{
	PHP_MD5_CTX md5_context;
	PHP_SHA1_CTX sha1_context;
#if defined(HAVE_HASH_EXT) && !defined(COMPILE_DL_HASH)
	void *hash_context = NULL;
#endif
	unsigned char *digest;
	int digest_len;
	int j;
	char *buf, *outid;
	struct timeval tv;
	zval **array;
	zval **token;
	char *remote_addr = NULL;

	gettimeofday(&tv, NULL);

	if (zend_hash_find(&EG(symbol_table), "_SERVER", sizeof("_SERVER"), (void **) &array) == SUCCESS &&
		Z_TYPE_PP(array) == IS_ARRAY &&
		zend_hash_find(Z_ARRVAL_PP(array), "REMOTE_ADDR", sizeof("REMOTE_ADDR"), (void **) &token) == SUCCESS &&
		Z_TYPE_PP(token) == IS_STRING
	) {
		remote_addr = Z_STRVAL_PP(token);
	}

	/* maximum 15+19+19+10 bytes */
	spprintf(&buf, 0, "%.15s%ld%ld%0.8F", remote_addr ? remote_addr : "", tv.tv_sec, (long int)tv.tv_usec, php_combined_lcg(TSRMLS_C) * 10);

	switch (PS(hash_func)) {
		case PS_HASH_FUNC_MD5:
			PHP_MD5Init(&md5_context);
			PHP_MD5Update(&md5_context, (unsigned char *) buf, strlen(buf));
			digest_len = 16;
			break;
		case PS_HASH_FUNC_SHA1:
			PHP_SHA1Init(&sha1_context);
			PHP_SHA1Update(&sha1_context, (unsigned char *) buf, strlen(buf));
			digest_len = 20;
			break;
#if defined(HAVE_HASH_EXT) && !defined(COMPILE_DL_HASH)
		case PS_HASH_FUNC_OTHER:
			if (!PS(hash_ops)) {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid session hash function");
				efree(buf);
				return NULL;
			}

			hash_context = emalloc(PS(hash_ops)->context_size);
			PS(hash_ops)->hash_init(hash_context);
			PS(hash_ops)->hash_update(hash_context, (unsigned char *) buf, strlen(buf));
			digest_len = PS(hash_ops)->digest_size;
			break;
#endif /* HAVE_HASH_EXT */
		default:
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid session hash function");
			efree(buf);
			return NULL;
	}
	efree(buf);

	if (PS(entropy_length) > 0) {
#ifdef PHP_WIN32
		unsigned char rbuf[2048];
		size_t toread = PS(entropy_length);

		if (php_win32_get_random_bytes(rbuf, MIN(toread, sizeof(rbuf))) == SUCCESS){

			switch (PS(hash_func)) {
				case PS_HASH_FUNC_MD5:
					PHP_MD5Update(&md5_context, rbuf, toread);
					break;
				case PS_HASH_FUNC_SHA1:
					PHP_SHA1Update(&sha1_context, rbuf, toread);
					break;
# if defined(HAVE_HASH_EXT) && !defined(COMPILE_DL_HASH)
				case PS_HASH_FUNC_OTHER:
					PS(hash_ops)->hash_update(hash_context, rbuf, toread);
					break;
# endif /* HAVE_HASH_EXT */
			}
		}
#else
		int fd;

		fd = VCWD_OPEN(PS(entropy_file), O_RDONLY);
		if (fd >= 0) {
			unsigned char rbuf[2048];
			int n;
			int to_read = PS(entropy_length);

			while (to_read > 0) {
				n = read(fd, rbuf, MIN(to_read, sizeof(rbuf)));
				if (n <= 0) break;

				switch (PS(hash_func)) {
					case PS_HASH_FUNC_MD5:
						PHP_MD5Update(&md5_context, rbuf, n);
						break;
					case PS_HASH_FUNC_SHA1:
						PHP_SHA1Update(&sha1_context, rbuf, n);
						break;
#if defined(HAVE_HASH_EXT) && !defined(COMPILE_DL_HASH)
					case PS_HASH_FUNC_OTHER:
						PS(hash_ops)->hash_update(hash_context, rbuf, n);
						break;
#endif /* HAVE_HASH_EXT */
				}
				to_read -= n;
			}
			close(fd);
		}
#endif
	}

	digest = emalloc(digest_len + 1);
	switch (PS(hash_func)) {
		case PS_HASH_FUNC_MD5:
			PHP_MD5Final(digest, &md5_context);
			break;
		case PS_HASH_FUNC_SHA1:
			PHP_SHA1Final(digest, &sha1_context);
			break;
#if defined(HAVE_HASH_EXT) && !defined(COMPILE_DL_HASH)
		case PS_HASH_FUNC_OTHER:
			PS(hash_ops)->hash_final(digest, hash_context);
			efree(hash_context);
			break;
#endif /* HAVE_HASH_EXT */
	}

	if (PS(hash_bits_per_character) < 4
			|| PS(hash_bits_per_character) > 6) {
		PS(hash_bits_per_character) = 4;

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The ini setting hash_bits_per_character is out of range (should be 4, 5, or 6) - using 4 for now");
	}
	
	outid = emalloc((size_t)((digest_len + 2) * ((8.0f / PS(hash_bits_per_character)) + 0.5)));
	j = (int) (bin_to_readable((char *)digest, digest_len, outid, (char)PS(hash_bits_per_character)) - outid);
	efree(digest);

	if (newlen) {
		*newlen = j;
	}

	return outid;
}