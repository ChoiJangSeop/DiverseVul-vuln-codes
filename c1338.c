BOOL ber_read_contextual_tag(wStream* s, BYTE tag, int* length, BOOL pc)
{
	BYTE byte;

	if(Stream_GetRemainingLength(s) < 1)
		return FALSE;
	Stream_Read_UINT8(s, byte);

	if (byte != ((BER_CLASS_CTXT | BER_PC(pc)) | (BER_TAG_MASK & tag)))
	{
		Stream_Rewind(s, 1);
		return FALSE;
	}

	return ber_read_length(s, length);
}