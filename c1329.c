int credssp_skip_ts_password_creds(rdpCredssp* credssp)
{
	int length;
	int ts_password_creds_length = 0;

	length = ber_skip_octet_string(credssp->identity.DomainLength * 2);
	length += ber_skip_contextual_tag(length);
	ts_password_creds_length += length;

	length = ber_skip_octet_string(credssp->identity.UserLength * 2);
	length += ber_skip_contextual_tag(length);
	ts_password_creds_length += length;

	length = ber_skip_octet_string(credssp->identity.PasswordLength * 2);
	length += ber_skip_contextual_tag(length);
	ts_password_creds_length += length;

	length = ber_skip_sequence(ts_password_creds_length);

	return length;
}