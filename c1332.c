BOOL ber_read_enumerated(wStream* s, BYTE* enumerated, BYTE count)
{
	int length;

	if(!ber_read_universal_tag(s, BER_TAG_ENUMERATED, FALSE) ||
		!ber_read_length(s, &length))
		return FALSE;

	if (length != 1 || Stream_GetRemainingLength(s) < 1)
		return FALSE;

	Stream_Read_UINT8(s, *enumerated);

	/* check that enumerated value falls within expected range */
	if (*enumerated + 1 > count)
		return FALSE;

	return TRUE;
}