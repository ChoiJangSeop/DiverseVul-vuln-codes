int sldns_str2wire_str_buf(const char* str, uint8_t* rd, size_t* len)
{
	uint8_t ch = 0;
	size_t sl = 0;
	const char* s = str;
	/* skip length byte */
	if(*len < 1)
		return LDNS_WIREPARSE_ERR_BUFFER_TOO_SMALL;

	/* read characters */
	while(sldns_parse_char(&ch, &s)) {
		if(sl >= 255)
			return RET_ERR(LDNS_WIREPARSE_ERR_INVALID_STR, s-str);
		if(*len < sl+1)
			return RET_ERR(LDNS_WIREPARSE_ERR_BUFFER_TOO_SMALL,
				s-str);
		rd[++sl] = ch;
	}
	if(!s)
		return LDNS_WIREPARSE_ERR_SYNTAX_BAD_ESCAPE;
	rd[0] = (uint8_t)sl;
	*len = sl+1;
	return LDNS_WIREPARSE_ERR_OK;
}