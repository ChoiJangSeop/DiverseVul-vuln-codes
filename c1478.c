PHPAPI void php_session_start(TSRMLS_D) /* {{{ */
{
	zval **ppid;
	zval **data;
	char *p, *value;
	int nrand;
	int lensess;

	if (PS(use_only_cookies)) {
		PS(apply_trans_sid) = 0;
	} else {
		PS(apply_trans_sid) = PS(use_trans_sid);
	}

	switch (PS(session_status)) {
		case php_session_active:
			php_error(E_NOTICE, "A session had already been started - ignoring session_start()");
			return;
			break;

		case php_session_disabled:
			value = zend_ini_string("session.save_handler", sizeof("session.save_handler"), 0);
			if (!PS(mod) && value) {
				PS(mod) = _php_find_ps_module(value TSRMLS_CC);
				if (!PS(mod)) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot find save handler '%s' - session startup failed", value);
					return;
				}
			}
			value = zend_ini_string("session.serialize_handler", sizeof("session.serialize_handler"), 0);
			if (!PS(serializer) && value) {
				PS(serializer) = _php_find_ps_serializer(value TSRMLS_CC);
				if (!PS(serializer)) {
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot find serialization handler '%s' - session startup failed", value);
					return;
				}
			}
			PS(session_status) = php_session_none;
			/* fallthrough */

		default:
		case php_session_none:
			PS(define_sid) = 1;
			PS(send_cookie) = 1;
	}

	lensess = strlen(PS(session_name));

	/* Cookies are preferred, because initially
	 * cookie and get variables will be available. */

	if (!PS(id)) {
		if (PS(use_cookies) && zend_hash_find(&EG(symbol_table), "_COOKIE", sizeof("_COOKIE"), (void **) &data) == SUCCESS &&
				Z_TYPE_PP(data) == IS_ARRAY &&
				zend_hash_find(Z_ARRVAL_PP(data), PS(session_name), lensess + 1, (void **) &ppid) == SUCCESS
		) {
			PPID2SID;
			PS(apply_trans_sid) = 0;
			PS(send_cookie) = 0;
			PS(define_sid) = 0;
		}

		if (!PS(use_only_cookies) && !PS(id) &&
				zend_hash_find(&EG(symbol_table), "_GET", sizeof("_GET"), (void **) &data) == SUCCESS &&
				Z_TYPE_PP(data) == IS_ARRAY &&
				zend_hash_find(Z_ARRVAL_PP(data), PS(session_name), lensess + 1, (void **) &ppid) == SUCCESS
		) {
			PPID2SID;
			PS(send_cookie) = 0;
		}

		if (!PS(use_only_cookies) && !PS(id) &&
				zend_hash_find(&EG(symbol_table), "_POST", sizeof("_POST"), (void **) &data) == SUCCESS &&
				Z_TYPE_PP(data) == IS_ARRAY &&
				zend_hash_find(Z_ARRVAL_PP(data), PS(session_name), lensess + 1, (void **) &ppid) == SUCCESS
		) {
			PPID2SID;
			PS(send_cookie) = 0;
		}
	}

	/* Check the REQUEST_URI symbol for a string of the form
	 * '<session-name>=<session-id>' to allow URLs of the form
	 * http://yoursite/<session-name>=<session-id>/script.php */

	if (!PS(use_only_cookies) && !PS(id) && PG(http_globals)[TRACK_VARS_SERVER] &&
			zend_hash_find(Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]), "REQUEST_URI", sizeof("REQUEST_URI"), (void **) &data) == SUCCESS &&
			Z_TYPE_PP(data) == IS_STRING &&
			(p = strstr(Z_STRVAL_PP(data), PS(session_name))) &&
			p[lensess] == '='
	) {
		char *q;

		p += lensess + 1;
		if ((q = strpbrk(p, "/?\\"))) {
			PS(id) = estrndup(p, q - p);
			PS(send_cookie) = 0;
		}
	}

	/* Check whether the current request was referred to by
	 * an external site which invalidates the previously found id. */

	if (PS(id) &&
			PS(extern_referer_chk)[0] != '\0' &&
			PG(http_globals)[TRACK_VARS_SERVER] &&
			zend_hash_find(Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]), "HTTP_REFERER", sizeof("HTTP_REFERER"), (void **) &data) == SUCCESS &&
			Z_TYPE_PP(data) == IS_STRING &&
			Z_STRLEN_PP(data) != 0 &&
			strstr(Z_STRVAL_PP(data), PS(extern_referer_chk)) == NULL
	) {
		efree(PS(id));
		PS(id) = NULL;
		PS(send_cookie) = 1;
		if (PS(use_trans_sid) && !PS(use_only_cookies)) {
			PS(apply_trans_sid) = 1;
		}
	}

	php_session_initialize(TSRMLS_C);

	if (!PS(use_cookies) && PS(send_cookie)) {
		if (PS(use_trans_sid) && !PS(use_only_cookies)) {
			PS(apply_trans_sid) = 1;
		}
		PS(send_cookie) = 0;
	}

	php_session_reset_id(TSRMLS_C);

	PS(session_status) = php_session_active;

	php_session_cache_limiter(TSRMLS_C);

	if ((PS(mod_data) || PS(mod_user_implemented)) && PS(gc_probability) > 0) {
		int nrdels = -1;

		nrand = (int) ((float) PS(gc_divisor) * php_combined_lcg(TSRMLS_C));
		if (nrand < PS(gc_probability)) {
			PS(mod)->s_gc(&PS(mod_data), PS(gc_maxlifetime), &nrdels TSRMLS_CC);
#ifdef SESSION_DEBUG
			if (nrdels != -1) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "purged %d expired session objects", nrdels);
			}
#endif
		}
	}
}