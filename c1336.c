void credssp_write_ts_credentials(rdpCredssp* credssp, wStream* s)
{
	int length;
	int ts_password_creds_length;

	length = credssp_skip_ts_credentials(credssp);
	ts_password_creds_length = credssp_skip_ts_password_creds(credssp);

	/* TSCredentials (SEQUENCE) */
	length = ber_get_content_length(length);
	length -= ber_write_sequence_tag(s, length);

	/* [0] credType (INTEGER) */
	length -= ber_write_contextual_tag(s, 0, 3, TRUE);
	length -= ber_write_integer(s, 1);

	/* [1] credentials (OCTET STRING) */
	length -= 1;
	length -= ber_write_contextual_tag(s, 1, length, TRUE);
	length -= ber_write_octet_string_tag(s, ts_password_creds_length);

	credssp_write_ts_password_creds(credssp, s);
}