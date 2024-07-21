void credssp_write_ts_password_creds(rdpCredssp* credssp, wStream* s)
{
	int length;

	length = credssp_skip_ts_password_creds(credssp);

	/* TSPasswordCreds (SEQUENCE) */
	length = ber_get_content_length(length);
	ber_write_sequence_tag(s, length);

	/* [0] domainName (OCTET STRING) */
	ber_write_contextual_tag(s, 0, credssp->identity.DomainLength * 2 + 2, TRUE);
	ber_write_octet_string(s, (BYTE*) credssp->identity.Domain, credssp->identity.DomainLength * 2);

	/* [1] userName (OCTET STRING) */
	ber_write_contextual_tag(s, 1, credssp->identity.UserLength * 2 + 2, TRUE);
	ber_write_octet_string(s, (BYTE*) credssp->identity.User, credssp->identity.UserLength * 2);

	/* [2] password (OCTET STRING) */
	ber_write_contextual_tag(s, 2, credssp->identity.PasswordLength * 2 + 2, TRUE);
	ber_write_octet_string(s, (BYTE*) credssp->identity.Password, credssp->identity.PasswordLength * 2);
}