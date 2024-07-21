int ber_write_length(wStream* s, int length)
{
	if (length > 0x7F)
	{
		Stream_Write_UINT8(s, 0x82);
		Stream_Write_UINT16_BE(s, length);
		return 3;
	}
	else
	{
		Stream_Write_UINT8(s, length);
		return 1;
	}
}