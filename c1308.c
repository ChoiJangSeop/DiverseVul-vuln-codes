int credssp_skip_ts_credentials(rdpCredssp* credssp)
{
	int length;
	int ts_password_creds_length;
	int ts_credentials_length = 0;

	length = ber_skip_integer(0);
	length += ber_skip_contextual_tag(length);
	ts_credentials_length += length;

	ts_password_creds_length = credssp_skip_ts_password_creds(credssp);
	length = ber_skip_octet_string(ts_password_creds_length);
	length += ber_skip_contextual_tag(length);
	ts_credentials_length += length;

	length = ber_skip_sequence(ts_credentials_length);

	return length;
}