static void php_session_decode(const char *val, int vallen TSRMLS_DC) /* {{{ */
{
	if (!PS(serializer)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown session.serialize_handler. Failed to decode session object");
		return;
	}
	if (PS(serializer)->decode(val, vallen TSRMLS_CC) == FAILURE) {
		php_session_destroy(TSRMLS_C);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to decode session object. Session has been destroyed");
	}
}