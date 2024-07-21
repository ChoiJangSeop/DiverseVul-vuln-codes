static ZIPARCHIVE_METHOD(getStatusString)
{
	struct zip *intern;
	zval *this = getThis();
	int zep, syp, len;
	char error_string[128];

	if (!this) {
		RETURN_FALSE;
	}

	ZIP_FROM_OBJECT(intern, this);

	zip_error_get(intern, &zep, &syp);

	len = zip_error_to_str(error_string, 128, zep, syp);
	RETVAL_STRINGL(error_string, len, 1); 
}