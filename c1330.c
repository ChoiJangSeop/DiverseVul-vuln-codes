int credssp_skip_pub_key_auth(int length)
{
	length = ber_skip_octet_string(length);
	length += ber_skip_contextual_tag(length);
	return length;
}