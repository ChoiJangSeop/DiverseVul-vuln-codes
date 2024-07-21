int ber_write_sequence_tag(wStream* s, int length)
{
	Stream_Write_UINT8(s, (BER_CLASS_UNIV | BER_CONSTRUCT) | (BER_TAG_MASK & BER_TAG_SEQUENCE));
	return ber_write_length(s, length) + 1;
}