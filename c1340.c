void ber_write_universal_tag(wStream* s, BYTE tag, BOOL pc)
{
	Stream_Write_UINT8(s, (BER_CLASS_UNIV | BER_PC(pc)) | (BER_TAG_MASK & tag));
}