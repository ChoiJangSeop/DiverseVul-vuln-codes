BOOL ber_read_bit_string(wStream* s, int* length, BYTE* padding)
{
	if(!ber_read_universal_tag(s, BER_TAG_BIT_STRING, FALSE) ||
		!ber_read_length(s, length))
		return FALSE;

	if(Stream_GetRemainingLength(s) < 1)
		return FALSE;
	Stream_Read_UINT8(s, *padding);
	return TRUE;
}