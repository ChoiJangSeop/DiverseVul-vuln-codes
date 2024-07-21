pack_pack(int argc, VALUE *argv, VALUE ary)
{
    static const char nul10[] = "\0\0\0\0\0\0\0\0\0\0";
    static const char spc10[] = "          ";
    const char *p, *pend;
    VALUE fmt, opt = Qnil, res, from, associates = 0, buffer = 0;
    char type;
    long len, idx, plen;
    const char *ptr;
    int enc_info = 1;		/* 0 - BINARY, 1 - US-ASCII, 2 - UTF-8 */
#ifdef NATINT_PACK
    int natint;		/* native integer */
#endif
    int integer_size, bigendian_p;

    rb_scan_args(argc, argv, "10:", &fmt, &opt);

    StringValue(fmt);
    p = RSTRING_PTR(fmt);
    pend = p + RSTRING_LEN(fmt);
    if (!NIL_P(opt)) {
	static ID keyword_ids[1];
	if (!keyword_ids[0])
	    CONST_ID(keyword_ids[0], "buffer");

	rb_get_kwargs(opt, keyword_ids, 0, 1, &buffer);

	if (buffer != Qundef && !RB_TYPE_P(buffer, T_STRING))
	    rb_raise(rb_eTypeError, "buffer must be String, not %s", rb_obj_classname(buffer));
    }
    if (buffer)
	res = buffer;
    else
	res = rb_str_buf_new(0);

    idx = 0;

#define TOO_FEW (rb_raise(rb_eArgError, toofew), 0)
#define MORE_ITEM (idx < RARRAY_LEN(ary))
#define THISFROM (MORE_ITEM ? RARRAY_AREF(ary, idx) : TOO_FEW)
#define NEXTFROM (MORE_ITEM ? RARRAY_AREF(ary, idx++) : TOO_FEW)

    while (p < pend) {
	int explicit_endian = 0;
	if (RSTRING_PTR(fmt) + RSTRING_LEN(fmt) != pend) {
	    rb_raise(rb_eRuntimeError, "format string modified");
	}
	type = *p++;		/* get data type */
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

	if (*p == '*') {	/* set data length */
	    len = strchr("@Xxu", type) ? 0
                : strchr("PMm", type) ? 1
                : RARRAY_LEN(ary) - idx;
	    p++;
	}
	else if (ISDIGIT(*p)) {
	    errno = 0;
	    len = STRTOUL(p, (char**)&p, 10);
	    if (errno) {
		rb_raise(rb_eRangeError, "pack length too big");
	    }
	}
	else {
	    len = 1;
	}

	switch (type) {
	  case 'U':
	    /* if encoding is US-ASCII, upgrade to UTF-8 */
	    if (enc_info == 1) enc_info = 2;
	    break;
	  case 'm': case 'M': case 'u':
	    /* keep US-ASCII (do nothing) */
	    break;
	  default:
	    /* fall back to BINARY */
	    enc_info = 0;
	    break;
	}
	switch (type) {
	  case 'A': case 'a': case 'Z':
	  case 'B': case 'b':
	  case 'H': case 'h':
	    from = NEXTFROM;
	    if (NIL_P(from)) {
		ptr = "";
		plen = 0;
	    }
	    else {
		StringValue(from);
		ptr = RSTRING_PTR(from);
		plen = RSTRING_LEN(from);
		OBJ_INFECT(res, from);
	    }

	    if (p[-1] == '*')
		len = plen;

	    switch (type) {
	      case 'a':		/* arbitrary binary string (null padded)  */
	      case 'A':         /* arbitrary binary string (ASCII space padded) */
	      case 'Z':         /* null terminated string  */
		if (plen >= len) {
		    rb_str_buf_cat(res, ptr, len);
		    if (p[-1] == '*' && type == 'Z')
			rb_str_buf_cat(res, nul10, 1);
		}
		else {
		    rb_str_buf_cat(res, ptr, plen);
		    len -= plen;
		    while (len >= 10) {
			rb_str_buf_cat(res, (type == 'A')?spc10:nul10, 10);
			len -= 10;
		    }
		    rb_str_buf_cat(res, (type == 'A')?spc10:nul10, len);
		}
		break;

#define castchar(from) (char)((from) & 0xff)

	      case 'b':		/* bit string (ascending) */
		{
		    int byte = 0;
		    long i, j = 0;

		    if (len > plen) {
			j = (len - plen + 1)/2;
			len = plen;
		    }
		    for (i=0; i++ < len; ptr++) {
			if (*ptr & 1)
			    byte |= 128;
			if (i & 7)
			    byte >>= 1;
			else {
			    char c = castchar(byte);
			    rb_str_buf_cat(res, &c, 1);
			    byte = 0;
			}
		    }
		    if (len & 7) {
			char c;
			byte >>= 7 - (len & 7);
			c = castchar(byte);
			rb_str_buf_cat(res, &c, 1);
		    }
		    len = j;
		    goto grow;
		}
		break;

	      case 'B':		/* bit string (descending) */
		{
		    int byte = 0;
		    long i, j = 0;

		    if (len > plen) {
			j = (len - plen + 1)/2;
			len = plen;
		    }
		    for (i=0; i++ < len; ptr++) {
			byte |= *ptr & 1;
			if (i & 7)
			    byte <<= 1;
			else {
			    char c = castchar(byte);
			    rb_str_buf_cat(res, &c, 1);
			    byte = 0;
			}
		    }
		    if (len & 7) {
			char c;
			byte <<= 7 - (len & 7);
			c = castchar(byte);
			rb_str_buf_cat(res, &c, 1);
		    }
		    len = j;
		    goto grow;
		}
		break;

	      case 'h':		/* hex string (low nibble first) */
		{
		    int byte = 0;
		    long i, j = 0;

		    if (len > plen) {
			j = (len + 1) / 2 - (plen + 1) / 2;
			len = plen;
		    }
		    for (i=0; i++ < len; ptr++) {
			if (ISALPHA(*ptr))
			    byte |= (((*ptr & 15) + 9) & 15) << 4;
			else
			    byte |= (*ptr & 15) << 4;
			if (i & 1)
			    byte >>= 4;
			else {
			    char c = castchar(byte);
			    rb_str_buf_cat(res, &c, 1);
			    byte = 0;
			}
		    }
		    if (len & 1) {
			char c = castchar(byte);
			rb_str_buf_cat(res, &c, 1);
		    }
		    len = j;
		    goto grow;
		}
		break;

	      case 'H':		/* hex string (high nibble first) */
		{
		    int byte = 0;
		    long i, j = 0;

		    if (len > plen) {
			j = (len + 1) / 2 - (plen + 1) / 2;
			len = plen;
		    }
		    for (i=0; i++ < len; ptr++) {
			if (ISALPHA(*ptr))
			    byte |= ((*ptr & 15) + 9) & 15;
			else
			    byte |= *ptr & 15;
			if (i & 1)
			    byte <<= 4;
			else {
			    char c = castchar(byte);
			    rb_str_buf_cat(res, &c, 1);
			    byte = 0;
			}
		    }
		    if (len & 1) {
			char c = castchar(byte);
			rb_str_buf_cat(res, &c, 1);
		    }
		    len = j;
		    goto grow;
		}
		break;
	    }
	    break;

	  case 'c':		/* signed char */
	  case 'C':		/* unsigned char */
            integer_size = 1;
            bigendian_p = BIGENDIAN_P(); /* not effective */
            goto pack_integer;

	  case 's':		/* s for int16_t, s! for signed short */
            integer_size = NATINT_LEN(short, 2);
            bigendian_p = BIGENDIAN_P();
            goto pack_integer;

	  case 'S':		/* S for uint16_t, S! for unsigned short */
            integer_size = NATINT_LEN(short, 2);
            bigendian_p = BIGENDIAN_P();
            goto pack_integer;

	  case 'i':		/* i and i! for signed int */
            integer_size = (int)sizeof(int);
            bigendian_p = BIGENDIAN_P();
            goto pack_integer;

	  case 'I':		/* I and I! for unsigned int */
            integer_size = (int)sizeof(int);
            bigendian_p = BIGENDIAN_P();
            goto pack_integer;

	  case 'l':		/* l for int32_t, l! for signed long */
            integer_size = NATINT_LEN(long, 4);
            bigendian_p = BIGENDIAN_P();
            goto pack_integer;

	  case 'L':		/* L for uint32_t, L! for unsigned long */
            integer_size = NATINT_LEN(long, 4);
            bigendian_p = BIGENDIAN_P();
            goto pack_integer;

	  case 'q':		/* q for int64_t, q! for signed long long */
	    integer_size = NATINT_LEN_Q;
            bigendian_p = BIGENDIAN_P();
            goto pack_integer;

	  case 'Q':		/* Q for uint64_t, Q! for unsigned long long */
	    integer_size = NATINT_LEN_Q;
            bigendian_p = BIGENDIAN_P();
            goto pack_integer;

	  case 'j':		/* j for intptr_t */
	    integer_size = sizeof(intptr_t);
	    bigendian_p = BIGENDIAN_P();
	    goto pack_integer;

	  case 'J':		/* J for uintptr_t */
	    integer_size = sizeof(uintptr_t);
	    bigendian_p = BIGENDIAN_P();
	    goto pack_integer;

	  case 'n':		/* 16 bit (2 bytes) integer (network byte-order)  */
            integer_size = 2;
            bigendian_p = 1;
            goto pack_integer;

	  case 'N':		/* 32 bit (4 bytes) integer (network byte-order) */
            integer_size = 4;
            bigendian_p = 1;
            goto pack_integer;

	  case 'v':		/* 16 bit (2 bytes) integer (VAX byte-order) */
            integer_size = 2;
            bigendian_p = 0;
            goto pack_integer;

	  case 'V':		/* 32 bit (4 bytes) integer (VAX byte-order) */
            integer_size = 4;
            bigendian_p = 0;
            goto pack_integer;

          pack_integer:
	    if (explicit_endian) {
		bigendian_p = explicit_endian == '>';
	    }
            if (integer_size > MAX_INTEGER_PACK_SIZE)
                rb_bug("unexpected intger size for pack: %d", integer_size);
            while (len-- > 0) {
                char intbuf[MAX_INTEGER_PACK_SIZE];

                from = NEXTFROM;
                rb_integer_pack(from, intbuf, integer_size, 1, 0,
                    INTEGER_PACK_2COMP |
                    (bigendian_p ? INTEGER_PACK_BIG_ENDIAN : INTEGER_PACK_LITTLE_ENDIAN));
                rb_str_buf_cat(res, intbuf, integer_size);
            }
	    break;

	  case 'f':		/* single precision float in native format */
	  case 'F':		/* ditto */
	    while (len-- > 0) {
		float f;

		from = NEXTFROM;
		f = (float)RFLOAT_VALUE(rb_to_float(from));
		rb_str_buf_cat(res, (char*)&f, sizeof(float));
	    }
	    break;

	  case 'e':		/* single precision float in VAX byte-order */
	    while (len-- > 0) {
		FLOAT_CONVWITH(tmp);

		from = NEXTFROM;
		tmp.f = (float)RFLOAT_VALUE(rb_to_float(from));
		HTOVF(tmp);
		rb_str_buf_cat(res, tmp.buf, sizeof(float));
	    }
	    break;

	  case 'E':		/* double precision float in VAX byte-order */
	    while (len-- > 0) {
		DOUBLE_CONVWITH(tmp);
		from = NEXTFROM;
		tmp.d = RFLOAT_VALUE(rb_to_float(from));
		HTOVD(tmp);
		rb_str_buf_cat(res, tmp.buf, sizeof(double));
	    }
	    break;

	  case 'd':		/* double precision float in native format */
	  case 'D':		/* ditto */
	    while (len-- > 0) {
		double d;

		from = NEXTFROM;
		d = RFLOAT_VALUE(rb_to_float(from));
		rb_str_buf_cat(res, (char*)&d, sizeof(double));
	    }
	    break;

	  case 'g':		/* single precision float in network byte-order */
	    while (len-- > 0) {
		FLOAT_CONVWITH(tmp);
		from = NEXTFROM;
		tmp.f = (float)RFLOAT_VALUE(rb_to_float(from));
		HTONF(tmp);
		rb_str_buf_cat(res, tmp.buf, sizeof(float));
	    }
	    break;

	  case 'G':		/* double precision float in network byte-order */
	    while (len-- > 0) {
		DOUBLE_CONVWITH(tmp);

		from = NEXTFROM;
		tmp.d = RFLOAT_VALUE(rb_to_float(from));
		HTOND(tmp);
		rb_str_buf_cat(res, tmp.buf, sizeof(double));
	    }
	    break;

	  case 'x':		/* null byte */
	  grow:
	    while (len >= 10) {
		rb_str_buf_cat(res, nul10, 10);
		len -= 10;
	    }
	    rb_str_buf_cat(res, nul10, len);
	    break;

	  case 'X':		/* back up byte */
	  shrink:
	    plen = RSTRING_LEN(res);
	    if (plen < len)
		rb_raise(rb_eArgError, "X outside of string");
	    rb_str_set_len(res, plen - len);
	    break;

	  case '@':		/* null fill to absolute position */
	    len -= RSTRING_LEN(res);
	    if (len > 0) goto grow;
	    len = -len;
	    if (len > 0) goto shrink;
	    break;

	  case '%':
	    rb_raise(rb_eArgError, "%% is not supported");
	    break;

	  case 'U':		/* Unicode character */
	    while (len-- > 0) {
		SIGNED_VALUE l;
		char buf[8];
		int le;

		from = NEXTFROM;
		from = rb_to_int(from);
		l = NUM2LONG(from);
		if (l < 0) {
		    rb_raise(rb_eRangeError, "pack(U): value out of range");
		}
		le = rb_uv_to_utf8(buf, l);
		rb_str_buf_cat(res, (char*)buf, le);
	    }
	    break;

	  case 'u':		/* uuencoded string */
	  case 'm':		/* base64 encoded string */
	    from = NEXTFROM;
	    StringValue(from);
	    ptr = RSTRING_PTR(from);
	    plen = RSTRING_LEN(from);

	    if (len == 0 && type == 'm') {
		encodes(res, ptr, plen, type, 0);
		ptr += plen;
		break;
	    }
	    if (len <= 2)
		len = 45;
	    else if (len > 63 && type == 'u')
		len = 63;
	    else
		len = len / 3 * 3;
	    while (plen > 0) {
		long todo;

		if (plen > len)
		    todo = len;
		else
		    todo = plen;
		encodes(res, ptr, todo, type, 1);
		plen -= todo;
		ptr += todo;
	    }
	    break;

	  case 'M':		/* quoted-printable encoded string */
	    from = rb_obj_as_string(NEXTFROM);
	    if (len <= 1)
		len = 72;
	    qpencode(res, from, len);
	    break;

	  case 'P':		/* pointer to packed byte string */
	    from = THISFROM;
	    if (!NIL_P(from)) {
		StringValue(from);
		if (RSTRING_LEN(from) < len) {
		    rb_raise(rb_eArgError, "too short buffer for P(%ld for %ld)",
			     RSTRING_LEN(from), len);
		}
	    }
	    len = 1;
	    /* FALL THROUGH */
	  case 'p':		/* pointer to string */
	    while (len-- > 0) {
		char *t;
		from = NEXTFROM;
		if (NIL_P(from)) {
		    t = 0;
		}
		else {
		    t = StringValuePtr(from);
		    rb_obj_taint(from);
		}
		if (!associates) {
		    associates = rb_ary_new();
		}
		rb_ary_push(associates, from);
		rb_str_buf_cat(res, (char*)&t, sizeof(char*));
	    }
	    break;

	  case 'w':		/* BER compressed integer  */
	    while (len-- > 0) {
		VALUE buf = rb_str_new(0, 0);
                size_t numbytes;
                int sign;
                char *cp;

		from = NEXTFROM;
                from = rb_to_int(from);
                numbytes = rb_absint_numwords(from, 7, NULL);
                if (numbytes == 0)
                    numbytes = 1;
                buf = rb_str_new(NULL, numbytes);

                sign = rb_integer_pack(from, RSTRING_PTR(buf), RSTRING_LEN(buf), 1, 1, INTEGER_PACK_BIG_ENDIAN);

                if (sign < 0)
                    rb_raise(rb_eArgError, "can't compress negative numbers");
                if (sign == 2)
                    rb_bug("buffer size problem?");

                cp = RSTRING_PTR(buf);
                while (1 < numbytes) {
                  *cp |= 0x80;
                  cp++;
                  numbytes--;
                }

                rb_str_buf_cat(res, RSTRING_PTR(buf), RSTRING_LEN(buf));
	    }
	    break;

	  default: {
	    char unknown[5];
	    if (ISPRINT(type)) {
		unknown[0] = type;
		unknown[1] = '\0';
	    }
	    else {
		snprintf(unknown, sizeof(unknown), "\\x%.2x", type & 0xff);
	    }
	    rb_warning("unknown pack directive '%s' in '% "PRIsVALUE"'",
		       unknown, fmt);
	    break;
	  }
	}
    }

    if (associates) {
	str_associate(res, associates);
    }
    OBJ_INFECT(res, fmt);
    switch (enc_info) {
      case 1:
	ENCODING_CODERANGE_SET(res, rb_usascii_encindex(), ENC_CODERANGE_7BIT);
	break;
      case 2:
	rb_enc_set_index(res, rb_utf8_encindex());
	break;
      default:
	/* do nothing, keep ASCII-8BIT */
	break;
    }
    return res;
}