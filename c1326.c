int ber_skip_sequence(int length)
{
	return 1 + _ber_skip_length(length) + length;
}