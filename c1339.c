BOOL ber_read_sequence_tag(wStream* s, int* length)
{
	BYTE byte;

	if(Stream_GetRemainingLength(s) < 1)
		return FALSE;
	Stream_Read_UINT8(s, byte);

	if (byte != ((BER_CLASS_UNIV | BER_CONSTRUCT) | (BER_TAG_SEQUENCE_OF)))
		return FALSE;

	return ber_read_length(s, length);
}