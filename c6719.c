pack_unpack_internal(VALUE str, VALUE fmt, int mode)
{
#define hexdigits ruby_hexdigits
    char *s, *send;
    char *p, *pend;
    VALUE ary;
    char type;
    long len;
    AVOID_CC_BUG long tmp_len;
    int star;
#ifdef NATINT_PACK
    int natint;			/* native integer */
#endif
    int signed_p, integer_size, bigendian_p;
#define UNPACK_PUSH(item) do {\
	VALUE item_val = (item);\
	if ((mode) == UNPACK_BLOCK) {\
	    rb_yield(item_val);\
	}\
	else if ((mode) == UNPACK_ARRAY) {\
	    rb_ary_push(ary, item_val);\
	}\
	else /* if ((mode) == UNPACK_1) { */ {\
	    return item_val; \
	}\
    } while (0)

    StringValue(str);
    StringValue(fmt);
    s = RSTRING_PTR(str);
    send = s + RSTRING_LEN(str);
    p = RSTRING_PTR(fmt);
    pend = p + RSTRING_LEN(fmt);

    ary = mode == UNPACK_ARRAY ? rb_ary_new() : Qnil;
    while (p < pend) {
	int explicit_endian = 0;
	type = *p++;
#ifdef NATINT_PACK
	natint = 0;
#endif

	if (ISSPACE(type)) continue;
	if (type == '#') {
	    while ((p < pend) && (*p != '\n')) {
		p++;
	    }
	    continue;
	}

	star = 0;
	{
          modifiers:
	    switch (*p) {
	      case '_':
	      case '!':

		if (strchr(natstr, type)) {
#ifdef NATINT_PACK
		    natint = 1;
#endif
		    p++;
		}
		else {
		    rb_raise(rb_eArgError, "'%c' allowed only after types %s", *p, natstr);
		}
		goto modifiers;

	      case '<':
	      case '>':
		if (!strchr(endstr, type)) {
		    rb_raise(rb_eArgError, "'%c' allowed only after types %s", *p, endstr);
		}
		if (explicit_endian) {
		    rb_raise(rb_eRangeError, "Can't use both '<' and '>'");
		}
		explicit_endian = *p++;
		goto modifiers;
	    }
	}

	if (p >= pend)
	    len = 1;
	else if (*p == '*') {
	    star = 1;
	    len = send - s;
	    p++;
	}
	else if (ISDIGIT(*p)) {
	    errno = 0;
	    len = STRTOUL(p, (char**)&p, 10);
	    if (len < 0 || errno) {
		rb_raise(rb_eRangeError, "pack length too big");
	    }
	}
	else {
	    len = (type != '@');
	}

	switch (type) {
	  case '%':
	    rb_raise(rb_eArgError, "%% is not supported");
	    break;

	  case 'A':
	    if (len > send - s) len = send - s;
	    {
		long end = len;
		char *t = s + len - 1;

		while (t >= s) {
		    if (*t != ' ' && *t != '\0') break;
		    t--; len--;
		}
		UNPACK_PUSH(infected_str_new(s, len, str));
		s += end;
	    }
	    break;

	  case 'Z':
	    {
		char *t = s;

		if (len > send-s) len = send-s;
		while (t < s+len && *t) t++;
		UNPACK_PUSH(infected_str_new(s, t-s, str));
		if (t < send) t++;
		s = star ? t : s+len;
	    }
	    break;

	  case 'a':
	    if (len > send - s) len = send - s;
	    UNPACK_PUSH(infected_str_new(s, len, str));
	    s += len;
	    break;

	  case 'b':
	    {
		VALUE bitstr;
		char *t;
		int bits;
		long i;

		if (p[-1] == '*' || len > (send - s) * 8)
		    len = (send - s) * 8;
		bits = 0;
		bitstr = rb_usascii_str_new(0, len);
		t = RSTRING_PTR(bitstr);
		for (i=0; i<len; i++) {
		    if (i & 7) bits >>= 1;
		    else bits = (unsigned char)*s++;
		    *t++ = (bits & 1) ? '1' : '0';
		}
		UNPACK_PUSH(bitstr);
	    }
	    break;

	  case 'B':
	    {
		VALUE bitstr;
		char *t;
		int bits;
		long i;

		if (p[-1] == '*' || len > (send - s) * 8)
		    len = (send - s) * 8;
		bits = 0;
		bitstr = rb_usascii_str_new(0, len);
		t = RSTRING_PTR(bitstr);
		for (i=0; i<len; i++) {
		    if (i & 7) bits <<= 1;
		    else bits = (unsigned char)*s++;
		    *t++ = (bits & 128) ? '1' : '0';
		}
		UNPACK_PUSH(bitstr);
	    }
	    break;

	  case 'h':
	    {
		VALUE bitstr;
		char *t;
		int bits;
		long i;

		if (p[-1] == '*' || len > (send - s) * 2)
		    len = (send - s) * 2;
		bits = 0;
		bitstr = rb_usascii_str_new(0, len);
		t = RSTRING_PTR(bitstr);
		for (i=0; i<len; i++) {
		    if (i & 1)
			bits >>= 4;
		    else
			bits = (unsigned char)*s++;
		    *t++ = hexdigits[bits & 15];
		}
		UNPACK_PUSH(bitstr);
	    }
	    break;

	  case 'H':
	    {
		VALUE bitstr;
		char *t;
		int bits;
		long i;

		if (p[-1] == '*' || len > (send - s) * 2)
		    len = (send - s) * 2;
		bits = 0;
		bitstr = rb_usascii_str_new(0, len);
		t = RSTRING_PTR(bitstr);
		for (i=0; i<len; i++) {
		    if (i & 1)
			bits <<= 4;
		    else
			bits = (unsigned char)*s++;
		    *t++ = hexdigits[(bits >> 4) & 15];
		}
		UNPACK_PUSH(bitstr);
	    }
	    break;

	  case 'c':
	    signed_p = 1;
	    integer_size = 1;
	    bigendian_p = BIGENDIAN_P(); /* not effective */
	    goto unpack_integer;

	  case 'C':
	    signed_p = 0;
	    integer_size = 1;
	    bigendian_p = BIGENDIAN_P(); /* not effective */
	    goto unpack_integer;

	  case 's':
	    signed_p = 1;
	    integer_size = NATINT_LEN(short, 2);
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'S':
	    signed_p = 0;
	    integer_size = NATINT_LEN(short, 2);
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'i':
	    signed_p = 1;
	    integer_size = (int)sizeof(int);
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'I':
	    signed_p = 0;
	    integer_size = (int)sizeof(int);
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'l':
	    signed_p = 1;
	    integer_size = NATINT_LEN(long, 4);
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'L':
	    signed_p = 0;
	    integer_size = NATINT_LEN(long, 4);
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'q':
	    signed_p = 1;
	    integer_size = NATINT_LEN_Q;
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'Q':
	    signed_p = 0;
	    integer_size = NATINT_LEN_Q;
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'j':
	    signed_p = 1;
	    integer_size = sizeof(intptr_t);
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'J':
	    signed_p = 0;
	    integer_size = sizeof(uintptr_t);
	    bigendian_p = BIGENDIAN_P();
	    goto unpack_integer;

	  case 'n':
	    signed_p = 0;
	    integer_size = 2;
	    bigendian_p = 1;
	    goto unpack_integer;

	  case 'N':
	    signed_p = 0;
	    integer_size = 4;
	    bigendian_p = 1;
	    goto unpack_integer;

	  case 'v':
	    signed_p = 0;
	    integer_size = 2;
	    bigendian_p = 0;
	    goto unpack_integer;

	  case 'V':
	    signed_p = 0;
	    integer_size = 4;
	    bigendian_p = 0;
	    goto unpack_integer;

	  unpack_integer:
	    if (explicit_endian) {
		bigendian_p = explicit_endian == '>';
	    }
            PACK_LENGTH_ADJUST_SIZE(integer_size);
            while (len-- > 0) {
                int flags = bigendian_p ? INTEGER_PACK_BIG_ENDIAN : INTEGER_PACK_LITTLE_ENDIAN;
                VALUE val;
                if (signed_p)
                    flags |= INTEGER_PACK_2COMP;
                val = rb_integer_unpack(s, integer_size, 1, 0, flags);
                UNPACK_PUSH(val);
                s += integer_size;
            }
            PACK_ITEM_ADJUST();
            break;

	  case 'f':
	  case 'F':
	    PACK_LENGTH_ADJUST_SIZE(sizeof(float));
	    while (len-- > 0) {
		float tmp;
		memcpy(&tmp, s, sizeof(float));
		s += sizeof(float);
		UNPACK_PUSH(DBL2NUM((double)tmp));
	    }
	    PACK_ITEM_ADJUST();
	    break;

	  case 'e':
	    PACK_LENGTH_ADJUST_SIZE(sizeof(float));
	    while (len-- > 0) {
		FLOAT_CONVWITH(tmp);
		memcpy(tmp.buf, s, sizeof(float));
		s += sizeof(float);
		VTOHF(tmp);
		UNPACK_PUSH(DBL2NUM(tmp.f));
	    }
	    PACK_ITEM_ADJUST();
	    break;

	  case 'E':
	    PACK_LENGTH_ADJUST_SIZE(sizeof(double));
	    while (len-- > 0) {
		DOUBLE_CONVWITH(tmp);
		memcpy(tmp.buf, s, sizeof(double));
		s += sizeof(double);
		VTOHD(tmp);
		UNPACK_PUSH(DBL2NUM(tmp.d));
	    }
	    PACK_ITEM_ADJUST();
	    break;

	  case 'D':
	  case 'd':
	    PACK_LENGTH_ADJUST_SIZE(sizeof(double));
	    while (len-- > 0) {
		double tmp;
		memcpy(&tmp, s, sizeof(double));
		s += sizeof(double);
		UNPACK_PUSH(DBL2NUM(tmp));
	    }
	    PACK_ITEM_ADJUST();
	    break;

	  case 'g':
	    PACK_LENGTH_ADJUST_SIZE(sizeof(float));
	    while (len-- > 0) {
		FLOAT_CONVWITH(tmp);
		memcpy(tmp.buf, s, sizeof(float));
		s += sizeof(float);
		NTOHF(tmp);
		UNPACK_PUSH(DBL2NUM(tmp.f));
	    }
	    PACK_ITEM_ADJUST();
	    break;

	  case 'G':
	    PACK_LENGTH_ADJUST_SIZE(sizeof(double));
	    while (len-- > 0) {
		DOUBLE_CONVWITH(tmp);
		memcpy(tmp.buf, s, sizeof(double));
		s += sizeof(double);
		NTOHD(tmp);
		UNPACK_PUSH(DBL2NUM(tmp.d));
	    }
	    PACK_ITEM_ADJUST();
	    break;

	  case 'U':
	    if (len > send - s) len = send - s;
	    while (len > 0 && s < send) {
		long alen = send - s;
		unsigned long l;

		l = utf8_to_uv(s, &alen);
		s += alen; len--;
		UNPACK_PUSH(ULONG2NUM(l));
	    }
	    break;

	  case 'u':
	    {
		VALUE buf = infected_str_new(0, (send - s)*3/4, str);
		char *ptr = RSTRING_PTR(buf);
		long total = 0;

		while (s < send && (unsigned char)*s > ' ' && (unsigned char)*s < 'a') {
		    long a,b,c,d;
		    char hunk[3];

		    len = ((unsigned char)*s++ - ' ') & 077;

		    total += len;
		    if (total > RSTRING_LEN(buf)) {
			len -= total - RSTRING_LEN(buf);
			total = RSTRING_LEN(buf);
		    }

		    while (len > 0) {
			long mlen = len > 3 ? 3 : len;

			if (s < send && (unsigned char)*s >= ' ' && (unsigned char)*s < 'a')
			    a = ((unsigned char)*s++ - ' ') & 077;
			else
			    a = 0;
			if (s < send && (unsigned char)*s >= ' ' && (unsigned char)*s < 'a')
			    b = ((unsigned char)*s++ - ' ') & 077;
			else
			    b = 0;
			if (s < send && (unsigned char)*s >= ' ' && (unsigned char)*s < 'a')
			    c = ((unsigned char)*s++ - ' ') & 077;
			else
			    c = 0;
			if (s < send && (unsigned char)*s >= ' ' && (unsigned char)*s < 'a')
			    d = ((unsigned char)*s++ - ' ') & 077;
			else
			    d = 0;
			hunk[0] = (char)(a << 2 | b >> 4);
			hunk[1] = (char)(b << 4 | c >> 2);
			hunk[2] = (char)(c << 6 | d);
			memcpy(ptr, hunk, mlen);
			ptr += mlen;
			len -= mlen;
		    }
		    if (s < send && (unsigned char)*s != '\r' && *s != '\n')
			s++;	/* possible checksum byte */
		    if (s < send && *s == '\r') s++;
		    if (s < send && *s == '\n') s++;
		}

		rb_str_set_len(buf, total);
		UNPACK_PUSH(buf);
	    }
	    break;

	  case 'm':
	    {
		VALUE buf = infected_str_new(0, (send - s + 3)*3/4, str); /* +3 is for skipping paddings */
		char *ptr = RSTRING_PTR(buf);
		int a = -1,b = -1,c = 0,d = 0;
		static signed char b64_xtable[256];

		if (b64_xtable['/'] <= 0) {
		    int i;

		    for (i = 0; i < 256; i++) {
			b64_xtable[i] = -1;
		    }
		    for (i = 0; i < 64; i++) {
			b64_xtable[(unsigned char)b64_table[i]] = (char)i;
		    }
		}
		if (len == 0) {
		    while (s < send) {
			a = b = c = d = -1;
			a = b64_xtable[(unsigned char)*s++];
			if (s >= send || a == -1) rb_raise(rb_eArgError, "invalid base64");
			b = b64_xtable[(unsigned char)*s++];
			if (s >= send || b == -1) rb_raise(rb_eArgError, "invalid base64");
			if (*s == '=') {
			    if (s + 2 == send && *(s + 1) == '=') break;
			    rb_raise(rb_eArgError, "invalid base64");
			}
			c = b64_xtable[(unsigned char)*s++];
			if (s >= send || c == -1) rb_raise(rb_eArgError, "invalid base64");
			if (s + 1 == send && *s == '=') break;
			d = b64_xtable[(unsigned char)*s++];
			if (d == -1) rb_raise(rb_eArgError, "invalid base64");
			*ptr++ = castchar(a << 2 | b >> 4);
			*ptr++ = castchar(b << 4 | c >> 2);
			*ptr++ = castchar(c << 6 | d);
		    }
		    if (c == -1) {
			*ptr++ = castchar(a << 2 | b >> 4);
			if (b & 0xf) rb_raise(rb_eArgError, "invalid base64");
		    }
		    else if (d == -1) {
			*ptr++ = castchar(a << 2 | b >> 4);
			*ptr++ = castchar(b << 4 | c >> 2);
			if (c & 0x3) rb_raise(rb_eArgError, "invalid base64");
		    }
		}
		else {
		    while (s < send) {
			a = b = c = d = -1;
			while ((a = b64_xtable[(unsigned char)*s]) == -1 && s < send) {s++;}
			if (s >= send) break;
			s++;
			while ((b = b64_xtable[(unsigned char)*s]) == -1 && s < send) {s++;}
			if (s >= send) break;
			s++;
			while ((c = b64_xtable[(unsigned char)*s]) == -1 && s < send) {if (*s == '=') break; s++;}
			if (*s == '=' || s >= send) break;
			s++;
			while ((d = b64_xtable[(unsigned char)*s]) == -1 && s < send) {if (*s == '=') break; s++;}
			if (*s == '=' || s >= send) break;
			s++;
			*ptr++ = castchar(a << 2 | b >> 4);
			*ptr++ = castchar(b << 4 | c >> 2);
			*ptr++ = castchar(c << 6 | d);
			a = -1;
		    }
		    if (a != -1 && b != -1) {
			if (c == -1)
			    *ptr++ = castchar(a << 2 | b >> 4);
			else {
			    *ptr++ = castchar(a << 2 | b >> 4);
			    *ptr++ = castchar(b << 4 | c >> 2);
			}
		    }
		}
		rb_str_set_len(buf, ptr - RSTRING_PTR(buf));
		UNPACK_PUSH(buf);
	    }
	    break;

	  case 'M':
	    {
		VALUE buf = infected_str_new(0, send - s, str);
		char *ptr = RSTRING_PTR(buf), *ss = s;
		int csum = 0;
		int c1, c2;

		while (s < send) {
		    if (*s == '=') {
			if (++s == send) break;
			if (s+1 < send && *s == '\r' && *(s+1) == '\n')
			    s++;
			if (*s != '\n') {
			    if ((c1 = hex2num(*s)) == -1) break;
			    if (++s == send) break;
			    if ((c2 = hex2num(*s)) == -1) break;
			    csum |= *ptr++ = castchar(c1 << 4 | c2);
			}
		    }
		    else {
			csum |= *ptr++ = *s;
		    }
		    s++;
		    ss = s;
		}
		rb_str_set_len(buf, ptr - RSTRING_PTR(buf));
		rb_str_buf_cat(buf, ss, send-ss);
		csum = ISASCII(csum) ? ENC_CODERANGE_7BIT : ENC_CODERANGE_VALID;
		ENCODING_CODERANGE_SET(buf, rb_ascii8bit_encindex(), csum);
		UNPACK_PUSH(buf);
	    }
	    break;

	  case '@':
	    if (len > RSTRING_LEN(str))
		rb_raise(rb_eArgError, "@ outside of string");
	    s = RSTRING_PTR(str) + len;
	    break;

	  case 'X':
	    if (len > s - RSTRING_PTR(str))
		rb_raise(rb_eArgError, "X outside of string");
	    s -= len;
	    break;

	  case 'x':
	    if (len > send - s)
		rb_raise(rb_eArgError, "x outside of string");
	    s += len;
	    break;

	  case 'P':
	    if (sizeof(char *) <= (size_t)(send - s)) {
		VALUE tmp = Qnil;
		char *t;

		memcpy(&t, s, sizeof(char *));
		s += sizeof(char *);

		if (t) {
		    VALUE a;
		    const VALUE *p, *pend;

		    if (!(a = str_associated(str))) {
			rb_raise(rb_eArgError, "no associated pointer");
		    }
		    p = RARRAY_CONST_PTR(a);
		    pend = p + RARRAY_LEN(a);
		    while (p < pend) {
			if (RB_TYPE_P(*p, T_STRING) && RSTRING_PTR(*p) == t) {
			    if (len < RSTRING_LEN(*p)) {
				tmp = rb_tainted_str_new(t, len);
				str_associate(tmp, a);
			    }
			    else {
				tmp = *p;
			    }
			    break;
			}
			p++;
		    }
		    if (p == pend) {
			rb_raise(rb_eArgError, "non associated pointer");
		    }
		}
		UNPACK_PUSH(tmp);
	    }
	    break;

	  case 'p':
	    if (len > (long)((send - s) / sizeof(char *)))
		len = (send - s) / sizeof(char *);
	    while (len-- > 0) {
		if ((size_t)(send - s) < sizeof(char *))
		    break;
		else {
		    VALUE tmp = Qnil;
		    char *t;

		    memcpy(&t, s, sizeof(char *));
		    s += sizeof(char *);

		    if (t) {
			VALUE a;
			const VALUE *p, *pend;

			if (!(a = str_associated(str))) {
			    rb_raise(rb_eArgError, "no associated pointer");
			}
			p = RARRAY_CONST_PTR(a);
			pend = p + RARRAY_LEN(a);
			while (p < pend) {
			    if (RB_TYPE_P(*p, T_STRING) && RSTRING_PTR(*p) == t) {
				tmp = *p;
				break;
			    }
			    p++;
			}
			if (p == pend) {
			    rb_raise(rb_eArgError, "non associated pointer");
			}
		    }
		    UNPACK_PUSH(tmp);
		}
	    }
	    break;

	  case 'w':
	    {
                char *s0 = s;
                while (len > 0 && s < send) {
                    if (*s & 0x80) {
                        s++;
                    }
                    else {
                        s++;
                        UNPACK_PUSH(rb_integer_unpack(s0, s-s0, 1, 1, INTEGER_PACK_BIG_ENDIAN));
                        len--;
                        s0 = s;
                    }
                }
	    }
	    break;

	  default:
	    rb_warning("unknown unpack directive '%c' in '%s'",
		type, RSTRING_PTR(fmt));
	    break;
	}
    }

    return ary;
}