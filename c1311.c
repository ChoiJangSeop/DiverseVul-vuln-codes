int ber_skip_contextual_tag(int length)
{
	return _ber_skip_length(length) + 1;
}