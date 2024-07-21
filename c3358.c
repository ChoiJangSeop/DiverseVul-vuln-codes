AddTrustedKey(keyid_t keyno)
{
	/*
	 * We need to add a MD5-key in addition to setting the
	 * trust, because authhavekey() requires type != 0.
	 */
	MD5auth_setkey(keyno, KEYTYPE, NULL, 0);

	authtrust(keyno, TRUE);

	return;
}