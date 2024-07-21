int ber_write_integer(wStream* s, UINT32 value)
{
	ber_write_universal_tag(s, BER_TAG_INTEGER, FALSE);

	if (value <= 0xFF)
	{
		ber_write_length(s, 1);
		Stream_Write_UINT8(s, value);
		return 2;
	}
	else if (value < 0xFF80)
	{
		ber_write_length(s, 2);
		Stream_Write_UINT16_BE(s, value);
		return 3;
	}
	else if (value < 0xFF8000)
	{
		ber_write_length(s, 3);
		Stream_Write_UINT8(s, (value >> 16));
		Stream_Write_UINT16_BE(s, (value & 0xFFFF));
		return 4;
	}
	else if (value <= 0xFFFFFFFF)
	{
		ber_write_length(s, 4);
		Stream_Write_UINT32_BE(s, value);
		return 5;
	}

	return 0;
}