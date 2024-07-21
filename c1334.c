BOOL ber_read_universal_tag(wStream* s, BYTE tag, BOOL pc)
{
	BYTE byte;

	if(Stream_GetRemainingLength(s) < 1)
		return FALSE;
	Stream_Read_UINT8(s, byte);

	if (byte != (BER_CLASS_UNIV | BER_PC(pc) | (BER_TAG_MASK & tag)))
		return FALSE;

	return TRUE;
}