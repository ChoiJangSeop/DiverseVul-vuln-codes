int ber_write_octet_string_tag(wStream* s, int length)
{
	ber_write_universal_tag(s, BER_TAG_OCTET_STRING, FALSE);
	ber_write_length(s, length);
	return 1 + _ber_skip_length(length);
}