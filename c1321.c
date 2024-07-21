int ber_skip_sequence_tag(int length)
{
	return 1 + _ber_skip_length(length);
}