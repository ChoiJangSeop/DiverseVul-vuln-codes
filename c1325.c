int ber_skip_octet_string(int length)
{
	return 1 + _ber_skip_length(length) + length;
}