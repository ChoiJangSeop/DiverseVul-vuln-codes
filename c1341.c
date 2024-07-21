int credssp_skip_nego_token(int length)
{
	length = der_skip_octet_string(length);
	length += der_skip_contextual_tag(length);
	return length;
}