mbfl_strpos(
    mbfl_string *haystack,
    mbfl_string *needle,
    int offset,
    int reverse)
{
	int result;
	mbfl_string _haystack_u8, _needle_u8;
	const mbfl_string *haystack_u8, *needle_u8 = NULL;
	const unsigned char *u8_tbl;

	if (haystack == NULL || haystack->val == NULL || needle == NULL || needle->val == NULL) {
		return -8;
	}

	{
		const mbfl_encoding *u8_enc;
		u8_enc = mbfl_no2encoding(mbfl_no_encoding_utf8);
		if (u8_enc == NULL || u8_enc->mblen_table == NULL) {
			return -8;
		}
		u8_tbl = u8_enc->mblen_table;
	}

	if (haystack->no_encoding != mbfl_no_encoding_utf8) {
		mbfl_string_init(&_haystack_u8);
		haystack_u8 = mbfl_convert_encoding(haystack, &_haystack_u8, mbfl_no_encoding_utf8);
		if (haystack_u8 == NULL) {
			result = -4;
			goto out;
		}
	} else {
		haystack_u8 = haystack;
	}

	if (needle->no_encoding != mbfl_no_encoding_utf8) {
		mbfl_string_init(&_needle_u8);
		needle_u8 = mbfl_convert_encoding(needle, &_needle_u8, mbfl_no_encoding_utf8);
		if (needle_u8 == NULL) {
			result = -4;
			goto out;
		}
	} else {
		needle_u8 = needle;
	}

	if (needle_u8->len < 1) {
		result = -8;
		goto out;
	}

	result = -1;
	if (haystack_u8->len < needle_u8->len) {
		goto out;
	}

	if (!reverse) {
		unsigned int jtbl[1 << (sizeof(unsigned char) * 8)];
		unsigned int needle_u8_len = needle_u8->len;
		unsigned int i;
		const unsigned char *p, *q, *e;
		const unsigned char *haystack_u8_val = haystack_u8->val,
		                    *needle_u8_val = needle_u8->val;
		for (i = 0; i < sizeof(jtbl) / sizeof(*jtbl); ++i) {
			jtbl[i] = needle_u8_len + 1;
		}
		for (i = 0; i < needle_u8_len - 1; ++i) {
			jtbl[needle_u8_val[i]] = needle_u8_len - i;
		}
		e = haystack_u8_val + haystack_u8->len;
		p = haystack_u8_val;
		while (--offset >= 0) {
			if (p >= e) {
				result = -16;
				goto out;
			}
			p += u8_tbl[*p];
		}
		p += needle_u8_len;
		if (p > e) {
			goto out;
		}
		while (p <= e) {
			const unsigned char *pv = p;
			q = needle_u8_val + needle_u8_len;
			for (;;) {
				if (q == needle_u8_val) {
					result = 0;
					while (p > haystack_u8_val) {
						unsigned char c = *--p;
						if (c < 0x80) {
							++result;
						} else if ((c & 0xc0) != 0x80) {
							++result;
						}	
					}
					goto out;
				}
				if (*--q != *--p) {
					break;
				}
			}
			p += jtbl[*p];
			if (p <= pv) {
				p = pv + 1;
			}
		}
	} else {
		unsigned int jtbl[1 << (sizeof(unsigned char) * 8)];
		unsigned int needle_u8_len = needle_u8->len, needle_len = 0;
		unsigned int i;
		const unsigned char *p, *e, *q, *qe;
		const unsigned char *haystack_u8_val = haystack_u8->val,
		                    *needle_u8_val = needle_u8->val;
		for (i = 0; i < sizeof(jtbl) / sizeof(*jtbl); ++i) {
			jtbl[i] = needle_u8_len;
		}
		for (i = needle_u8_len - 1; i > 0; --i) {
			unsigned char c = needle_u8_val[i];
			jtbl[c] = i;
			if (c < 0x80) {
				++needle_len;
			} else if ((c & 0xc0) != 0x80) {
				++needle_len;
			}
		}
		{
			unsigned char c = needle_u8_val[0];
			if (c < 0x80) {
				++needle_len;
			} else if ((c & 0xc0) != 0x80) {
				++needle_len;
			}
		}
		e = haystack_u8_val;
		p = e + haystack_u8->len;
		qe = needle_u8_val + needle_u8_len;
		if (offset < 0) {
			if (-offset > needle_len) {
				offset += needle_len; 
				while (offset < 0) {
					unsigned char c;
					if (p <= e) {
						result = -16;
						goto out;
					}
					c = *(--p);
					if (c < 0x80) {
						++offset;
					} else if ((c & 0xc0) != 0x80) {
						++offset;
					}
				}
			}
		} else {
			const unsigned char *ee = haystack_u8_val + haystack_u8->len;
			while (--offset >= 0) {
				if (e >= ee) {
					result = -16;
					goto out;
				}
				e += u8_tbl[*e];
			}
		}
		if (p < e + needle_u8_len) {
			goto out;
		}
		p -= needle_u8_len;
		while (p >= e) {
			const unsigned char *pv = p;
			q = needle_u8_val;
			for (;;) {
				if (q == qe) {
					result = 0;
					p -= needle_u8_len;
					while (p > haystack_u8_val) {
						unsigned char c = *--p;
						if (c < 0x80) {
							++result;
						} else if ((c & 0xc0) != 0x80) {
							++result;
						}	
					}
					goto out;
				}
				if (*q != *p) {
					break;
				}
				++p, ++q;
			}
			p -= jtbl[*p];
			if (p >= pv) {
				p = pv - 1;
			}
		}
	}
out:
	if (haystack_u8 == &_haystack_u8) {
		mbfl_string_clear(&_haystack_u8);
	}
	if (needle_u8 == &_needle_u8) {
		mbfl_string_clear(&_needle_u8);
	}
	return result;
}