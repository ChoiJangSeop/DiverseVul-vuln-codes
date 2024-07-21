authusekey(
	keyid_t keyno,
	int keytype,
	const u_char *str
	)
{
	size_t	len;

	len = strlen((const char *)str);
	if (0 == len)
		return 0;

	MD5auth_setkey(keyno, keytype, str, len);
	return 1;
}