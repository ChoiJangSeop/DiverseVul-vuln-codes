PS_CREATE_SID_FUNC(files)
{
	char *sid;
	PS_FILES_DATA;

	sid = php_session_create_id((void **)&data, newlen TSRMLS_CC);

	return sid;
}