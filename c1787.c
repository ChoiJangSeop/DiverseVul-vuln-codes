chkpass_eq(PG_FUNCTION_ARGS)
{
	chkpass    *a1 = (chkpass *) PG_GETARG_POINTER(0);
	text	   *a2 = PG_GETARG_TEXT_PP(1);
	char		str[9];

	text_to_cstring_buffer(a2, str, sizeof(str));
	PG_RETURN_BOOL(strcmp(a1->password, crypt(str, a1->password)) == 0);
}