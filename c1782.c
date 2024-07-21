chkpass_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	chkpass    *result;
	char		mysalt[4];
	char	   *crypt_output;
	static char salt_chars[] =
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	/* special case to let us enter encrypted passwords */
	if (*str == ':')
	{
		result = (chkpass *) palloc(sizeof(chkpass));
		strlcpy(result->password, str + 1, 13 + 1);
		PG_RETURN_POINTER(result);
	}

	if (verify_pass(str) != 0)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("password \"%s\" is weak", str)));

	result = (chkpass *) palloc(sizeof(chkpass));

	mysalt[0] = salt_chars[random() & 0x3f];
	mysalt[1] = salt_chars[random() & 0x3f];
	mysalt[2] = 0;				/* technically the terminator is not necessary
								 * but I like to play safe */

	if ((crypt_output = crypt(str, mysalt)) == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("crypt() failed")));
	strcpy(result->password, crypt_output);

	PG_RETURN_POINTER(result);
}