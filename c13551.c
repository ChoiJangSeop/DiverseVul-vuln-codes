STATIC U8 *
S_reghop4(U8 *s, SSize_t off, const U8* llim, const U8* rlim)
{
    PERL_ARGS_ASSERT_REGHOP4;

    if (off >= 0) {
        while (off-- && s < rlim) {
            /* XXX could check well-formedness here */
            s += UTF8SKIP(s);
        }
    }
    else {
        while (off++ && s > llim) {
            s--;
            if (UTF8_IS_CONTINUED(*s)) {
                while (s > llim && UTF8_IS_CONTINUATION(*s))
                    s--;
            }
            /* XXX could check well-formedness here */
        }
    }
    return s;