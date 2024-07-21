PS_CREATE_SID_FUNC(mm)
{
	char *sid;
	PS_MM_DATA;

	sid = php_session_create_id((void **)&data, newlen TSRMLS_CC);

	return sid;
}