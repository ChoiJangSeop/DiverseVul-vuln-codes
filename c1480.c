static void php_session_reset_id(TSRMLS_D) /* {{{ */
{
	int module_number = PS(module_number);

	if (PS(use_cookies) && PS(send_cookie)) {
		php_session_send_cookie(TSRMLS_C);
		PS(send_cookie) = 0;
	}

	/* if the SID constant exists, destroy it. */
	zend_hash_del(EG(zend_constants), "sid", sizeof("sid"));

	if (PS(define_sid)) {
		smart_str var = {0};

		smart_str_appends(&var, PS(session_name));
		smart_str_appendc(&var, '=');
		smart_str_appends(&var, PS(id));
		smart_str_0(&var);
		REGISTER_STRINGL_CONSTANT("SID", var.c, var.len, 0);
	} else {
		REGISTER_STRINGL_CONSTANT("SID", STR_EMPTY_ALLOC(), 0, 0);
	}

	if (PS(apply_trans_sid)) {
		php_url_scanner_reset_vars(TSRMLS_C);
		php_url_scanner_add_var(PS(session_name), strlen(PS(session_name)), PS(id), strlen(PS(id)), 1 TSRMLS_CC);
	}
}