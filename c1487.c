PS_CREATE_SID_FUNC(files)
{
	char *sid;
	int maxfail = 3;
	PS_FILES_DATA;

	do {
		sid = php_session_create_id((void **)&data, newlen TSRMLS_CC);
		/* Check collision */
		if (ps_files_key_exists(data, sid TSRMLS_CC) == SUCCESS) {
			if (sid) {
				efree(sid);
				sid = NULL;
			}
			if (!(maxfail--)) {
				return NULL;
			}
		}
	} while(!sid);

	return sid;
}