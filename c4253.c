int puff(unsigned char *dest,           /* pointer to destination pointer */
         boost::uint32_t *destlen,        /* amount of output space */
         const unsigned char *source,         /* pointer to source data pointer */
         boost::uint32_t *sourcelen)      /* amount of input available */
{
    struct state s;             /* input/output state */
    int last, type;             /* block information */
    int err;                    /* return value */

    /* initialize output state */
    s.out = dest;
    s.outlen = *destlen;                /* ignored if dest is NIL */
    s.outcnt = 0;

    /* initialize input state */
    s.in = source;
    s.inlen = *sourcelen;
    s.incnt = 0;
    s.bitbuf = 0;
    s.bitcnt = 0;

    /* return if bits() or decode() tries to read past available input */
    if (setjmp(s.env) != 0)             /* if came back here via longjmp() */
        err = 2;                        /* then skip do-loop, return error */
    else {
        /* process blocks until last block or error */
        do {
            last = bits(&s, 1);         /* one if last block */
            type = bits(&s, 2);         /* block type 0..3 */
            err = type == 0 ? stored(&s) :
                  (type == 1 ? fixed(&s) :
                   (type == 2 ? dynamic(&s) :
                    -1));               /* type == 3, invalid */
            if (err != 0) break;        /* return with error */
        } while (!last);
    }

    /* update the lengths and return */
    if (err <= 0) {
        *destlen = s.outcnt;
        *sourcelen = s.incnt;
    }
    return err;
}