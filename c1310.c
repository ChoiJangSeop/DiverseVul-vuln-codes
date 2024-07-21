int _ber_skip_length(int length)
{
	if (length > 0x80)
		return 3;
	else
		return 1;
}