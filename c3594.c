char* FAST_FUNC dname_dec(const uint8_t *cstr, int clen, const char *pre)
{
	char *ret = ret; /* for compiler */
	char *dst = NULL;

	/* We make two passes over the cstr string. First, we compute
	 * how long the resulting string would be. Then we allocate a
	 * new buffer of the required length, and fill it in with the
	 * expanded content. The advantage of this approach is not
	 * having to deal with requiring callers to supply their own
	 * buffer, then having to check if it's sufficiently large, etc.
	 */
	while (1) {
		/* note: "return NULL" below are leak-safe since
		 * dst isn't allocated yet */
		const uint8_t *c;
		unsigned crtpos, retpos, depth, len;

		crtpos = retpos = depth = len = 0;
		while (crtpos < clen) {
			c = cstr + crtpos;

			if ((*c & NS_CMPRSFLGS) == NS_CMPRSFLGS) {
				/* pointer */
				if (crtpos + 2 > clen) /* no offset to jump to? abort */
					return NULL;
				if (retpos == 0) /* toplevel? save return spot */
					retpos = crtpos + 2;
				depth++;
				crtpos = ((c[0] & 0x3f) << 8) | c[1]; /* jump */
			} else if (*c) {
				/* label */
				if (crtpos + *c + 1 > clen) /* label too long? abort */
					return NULL;
				if (dst)
					memcpy(dst + len, c + 1, *c);
				len += *c + 1;
				crtpos += *c + 1;
				if (dst)
					dst[len - 1] = '.';
			} else {
				/* NUL: end of current domain name */
				if (retpos == 0) {
					/* toplevel? keep going */
					crtpos++;
				} else {
					/* return to toplevel saved spot */
					crtpos = retpos;
					retpos = depth = 0;
				}
				if (dst)
					dst[len - 1] = ' ';
			}

			if (depth > NS_MAXDNSRCH /* too many jumps? abort, it's a loop */
			 || len > NS_MAXDNAME * NS_MAXDNSRCH /* result too long? abort */
			) {
				return NULL;
			}
		}

		if (!len) /* expanded string has 0 length? abort */
			return NULL;

		if (!dst) { /* first pass? */
			/* allocate dst buffer and copy pre */
			unsigned plen = strlen(pre);
			ret = xmalloc(plen + len);
			dst = stpcpy(ret, pre);
		} else {
			dst[len - 1] = '\0';
			break;
		}
	}

	return ret;
}