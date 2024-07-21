STATIC U8 *
S_reghop3(U8 *s, SSize_t off, const U8* lim)
{
    /* return the position 'off' UTF-8 characters away from 's', forward if
     * 'off' >= 0, backwards if negative.  But don't go outside of position
     * 'lim', which better be < s  if off < 0 */

    PERL_ARGS_ASSERT_REGHOP3;

    if (off >= 0) {
	while (off-- && s < lim) {
	    /* XXX could check well-formedness here */
	    s += UTF8SKIP(s);
	}
    }
    else {
        while (off++ && s > lim) {
            s--;
            if (UTF8_IS_CONTINUED(*s)) {
                while (s > lim && UTF8_IS_CONTINUATION(*s))
                    s--;
	    }
            /* XXX could check well-formedness here */
	}
    }
    return s;