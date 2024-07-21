limited_rand(unsigned long limit)
{
    unsigned long mask = make_mask(limit);
    int i;
    unsigned long val;

  retry:
    val = 0;
    for (i = SIZEOF_LONG/4-1; 0 <= i; i--) {
        if (mask >> (i * 32)) {
            val |= genrand_int32() << (i * 32);
            val &= mask;
            if (limit < val)
                goto retry;
        }
    }
    return val;
}