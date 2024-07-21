local int stored(struct state *s)
{
    unsigned len;       /* length of stored block */

    /* discard leftover bits from current byte (assumes s->bitcnt < 8) */
    s->bitbuf = 0;
    s->bitcnt = 0;

    /* get length and check against its one's complement */
    if (s->incnt + 4 > s->inlen) return 2;      /* not enough input */
    len = s->in[s->incnt++];
    len |= s->in[s->incnt++] << 8;
    if (s->in[s->incnt++] != (~len & 0xff) ||
        s->in[s->incnt++] != ((~len >> 8) & 0xff))
        return -2;                              /* didn't match complement! */

    /* copy len bytes from in to out */
    if (s->incnt + len > s->inlen) return 2;    /* not enough input */
    if (s->out != NIL) {
        if (s->outcnt + len > s->outlen)
            return 1;                           /* not enough output space */
        while (len--)
            s->out[s->outcnt++] = s->in[s->incnt++];
    }
    else {                                      /* just scanning */
        s->outcnt += len;
        s->incnt += len;
    }

    /* done with a valid stored block */
    return 0;
}