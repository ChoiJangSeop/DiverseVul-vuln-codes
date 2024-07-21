int credssp_write_ts_password_creds(rdpCredssp* credssp, wStream* s)
{
	int size = 0;
	int innerSize = credssp_sizeof_ts_password_creds(credssp);

	if (innerSize > Stream_GetRemainingLength(s))
	{
		printf("\033[91m[ ERROR ] Not enough space allocated for ts_password_creds\033[0m");
	}

	/* TSPasswordCreds (SEQUENCE) */

	size += ber_write_sequence_tag(s, innerSize);

	/* [0] domainName (OCTET STRING) */
	size += ber_write_sequence_octet_string(s, 0, (BYTE*) credssp->identity.Domain, credssp->identity.DomainLength * 2);

	/* [1] userName (OCTET STRING) */
	size += ber_write_sequence_octet_string(s, 1, (BYTE*) credssp->identity.User, credssp->identity.UserLength * 2);

	/* [2] password (OCTET STRING) */
	size += ber_write_sequence_octet_string(s, 2, (BYTE*) credssp->identity.Password, credssp->identity.PasswordLength * 2);

	return size;
}