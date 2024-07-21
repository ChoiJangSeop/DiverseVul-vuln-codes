int ber_write_contextual_tag(wStream* s, BYTE tag, int length, BOOL pc)
{
	Stream_Write_UINT8(s, (BER_CLASS_CTXT | BER_PC(pc)) | (BER_TAG_MASK & tag));
	return ber_write_length(s, length) + 1;
}