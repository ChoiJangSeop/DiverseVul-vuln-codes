limited_big_rand(struct RBignum *limit)
{
    unsigned long mask, lim, rnd;
    struct RBignum *val;
    int i, len, boundary;

    len = (limit->len * SIZEOF_BDIGITS + 3) / 4;
    val = (struct RBignum *)rb_big_clone((VALUE)limit);
    val->sign = 1;
#if SIZEOF_BDIGITS == 2
# define BIG_GET32(big,i) (((BDIGIT *)(big)->digits)[(i)*2] | \
                           ((i)*2+1 < (big)->len ? (((BDIGIT *)(big)->digits)[(i)*2+1] << 16) \
                                                 : 0))
# define BIG_SET32(big,i,d) ((((BDIGIT *)(big)->digits)[(i)*2] = (d) & 0xffff), \
                             ((i)*2+1 < (big)->len ? (((BDIGIT *)(big)->digits)[(i)*2+1] = (d) >> 16) \
                                                   : 0))
#else
    /* SIZEOF_BDIGITS == 4 */
# define BIG_GET32(big,i) (((BDIGIT *)(big)->digits)[i])
# define BIG_SET32(big,i,d) (((BDIGIT *)(big)->digits)[i] = (d))
#endif
  retry:
    mask = 0;
    boundary = 1;
    for (i = len-1; 0 <= i; i--) {
        lim = BIG_GET32(limit, i);
        mask = mask ? 0xffffffff : make_mask(lim);
        if (mask) {
            rnd = genrand_int32() & mask;
            if (boundary) {
                if (lim < rnd)
                    goto retry;
                if (rnd < lim)
                    boundary = 0;
            }
        }
        else {
            rnd = 0;
        }
        BIG_SET32(val, i, rnd);
    }
    return rb_big_norm((VALUE)val);
}