STATIC U8 *
S_reghopmaybe3(U8* s, SSize_t off, const U8* lim)
{
    PERL_ARGS_ASSERT_REGHOPMAYBE3;

    if (off >= 0) {
	while (off-- && s < lim) {
	    /* XXX could check well-formedness here */
	    s += UTF8SKIP(s);
	}
	if (off >= 0)
	    return NULL;
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
	if (off <= 0)
	    return NULL;
    }
    return s;