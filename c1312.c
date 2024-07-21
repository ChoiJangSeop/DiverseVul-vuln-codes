int credssp_skip_ts_request(int length)
{
	length += ber_skip_integer(2);
	length += ber_skip_contextual_tag(3);
	length += der_skip_sequence_tag(length);
	return length;
}