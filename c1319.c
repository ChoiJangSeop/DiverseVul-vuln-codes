int credssp_skip_nego_tokens(int length)
{
	length = credssp_skip_nego_token(length);
	length += der_skip_sequence_tag(length);
	length += der_skip_sequence_tag(length);
	length += der_skip_contextual_tag(length);
	return length;
}