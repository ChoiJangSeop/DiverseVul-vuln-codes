BOOL ber_read_application_tag(wStream* s, BYTE tag, int* length)
{
	BYTE byte;

	if (tag > 30)
	{
		if(Stream_GetRemainingLength(s) < 1)
			return FALSE;
		Stream_Read_UINT8(s, byte);

		if (byte != ((BER_CLASS_APPL | BER_CONSTRUCT) | BER_TAG_MASK))
			return FALSE;

		if(Stream_GetRemainingLength(s) < 1)
			return FALSE;
		Stream_Read_UINT8(s, byte);

		if (byte != tag)
			return FALSE;

		return ber_read_length(s, length);
	}
	else
	{
		if(Stream_GetRemainingLength(s) < 1)
			return FALSE;
		Stream_Read_UINT8(s, byte);

		if (byte != ((BER_CLASS_APPL | BER_CONSTRUCT) | (BER_TAG_MASK & tag)))
			return FALSE;

		return ber_read_length(s, length);
	}

	return TRUE;
}