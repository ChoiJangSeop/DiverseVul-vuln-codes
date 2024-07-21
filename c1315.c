BOOL ber_read_length(wStream* s, int* length)
{
	BYTE byte;

	if(Stream_GetRemainingLength(s) < 1)
		return FALSE;
	Stream_Read_UINT8(s, byte);

	if (byte & 0x80)
	{
		byte &= ~(0x80);

		if(Stream_GetRemainingLength(s) < byte)
			return FALSE;

		if (byte == 1)
			Stream_Read_UINT8(s, *length);
		else if (byte == 2)
			Stream_Read_UINT16_BE(s, *length);
		else
			return FALSE;
	}
	else
	{
		*length = byte;
	}
	return TRUE;
}