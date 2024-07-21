static void php_session_initialize(TSRMLS_D) /* {{{ */
{
	char *val;
	int vallen;

	/* check session name for invalid characters */
	if (PS(id) && strpbrk(PS(id), "\r\n\t <>'\"\\")) {
		efree(PS(id));
		PS(id) = NULL;
	}

	if (!PS(mod)) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "No storage module chosen - failed to initialize session");
		return;
	}

	/* Open session handler first */
	if (PS(mod)->s_open(&PS(mod_data), PS(save_path), PS(session_name) TSRMLS_CC) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Failed to initialize storage module: %s (path: %s)", PS(mod)->s_name, PS(save_path));
		return;
	}

	/* If there is no ID, use session module to create one */
	if (!PS(id)) {
new_session:
		PS(id) = PS(mod)->s_create_sid(&PS(mod_data), NULL TSRMLS_CC);
		if (PS(use_cookies)) {
			PS(send_cookie) = 1;
		}
	}

	/* Read data */
	/* Question: if you create a SID here, should you also try to read data?
	 * I'm not sure, but while not doing so will remove one session operation
	 * it could prove usefull for those sites which wish to have "default"
	 * session information. */
	php_session_track_init(TSRMLS_C);
	PS(invalid_session_id) = 0;
	if (PS(mod)->s_read(&PS(mod_data), PS(id), &val, &vallen TSRMLS_CC) == SUCCESS) {
		php_session_decode(val, vallen TSRMLS_CC);
		efree(val);
	} else if (PS(invalid_session_id)) { /* address instances where the session read fails due to an invalid id */
		PS(invalid_session_id) = 0;
		efree(PS(id));
		PS(id) = NULL;
		goto new_session;
	}
}