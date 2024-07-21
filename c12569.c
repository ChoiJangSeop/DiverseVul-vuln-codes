char *gf_utf_get_utf8_string_from_bom(u8 *data, u32 size, char **out_ptr)
{
	u32 unicode_type = 0;
	*out_ptr = NULL;

	if (size>=5) {
		/*0: no unicode, 1: UTF-16BE, 2: UTF-16LE*/
		if ((data[0]==0xFF) && (data[1]==0xFE)) {
			if (!data[2] && !data[3]) {
				return NULL;
			} else {
				unicode_type = 2;
			}
		} else if ((data[0]==0xFE) && (data[1]==0xFF)) {
			if (!data[2] && !data[3]) {
				return NULL;
			} else {
				unicode_type = 1;
			}
		} else if ((data[0]==0xEF) && (data[1]==0xBB) && (data[2]==0xBF)) {
			return data+4;
		}
	}

	if (!unicode_type) return data;

	if (size%2) size--;
	u16 *str_wc = gf_malloc(size+2);
	u16 *srcwc;
	char *dst = gf_malloc(size+2);
	*out_ptr = dst;
	u32 i;
	for (i=0; i<size; i+=2) {
		u16 wchar=0;
		u8 c1 = data[i];
		u8 c2 = data[i+1];

		/*Little-endian order*/
		if (unicode_type==2) {
			if (c2) {
				wchar = c2;
				wchar <<=8;
				wchar |= c1;
			}
			else wchar = c1;
		} else {
			wchar = c1;
			if (c2) {
				wchar <<= 8;
				wchar |= c2;
			}
		}
		str_wc[i/2] = wchar;
	}
	str_wc[i/2] = 0;
	srcwc = str_wc;
	gf_utf8_wcstombs(dst, size, (const unsigned short **) &srcwc);
	gf_free(str_wc);

	return dst;
}