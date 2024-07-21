int ber_skip_integer(UINT32 value)
{
	if (value <= 0xFF)
	{
		return _ber_skip_length(1) + 2;
	}
	else if (value <= 0xFFFF)
	{
		return _ber_skip_length(2) + 3;
	}
	else if (value <= 0xFFFFFFFF)
	{
		return _ber_skip_length(4) + 5;
	}

	return 0;
}