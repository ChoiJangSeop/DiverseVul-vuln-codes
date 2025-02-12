fmtint(char **sbuffer,
       char **buffer,
       size_t *currlen,
       size_t *maxlen, LLONG value, int base, int min, int max, int flags)
{
    int signvalue = 0;
    const char *prefix = "";
    unsigned LLONG uvalue;
    char convert[DECIMAL_SIZE(value) + 3];
    int place = 0;
    int spadlen = 0;
    int zpadlen = 0;
    int caps = 0;

    if (max < 0)
        max = 0;
    uvalue = value;
    if (!(flags & DP_F_UNSIGNED)) {
        if (value < 0) {
            signvalue = '-';
            uvalue = -value;
        } else if (flags & DP_F_PLUS)
            signvalue = '+';
        else if (flags & DP_F_SPACE)
            signvalue = ' ';
    }
    if (flags & DP_F_NUM) {
        if (base == 8)
            prefix = "0";
        if (base == 16)
            prefix = "0x";
    }
    if (flags & DP_F_UP)
        caps = 1;
    do {
        convert[place++] = (caps ? "0123456789ABCDEF" : "0123456789abcdef")
            [uvalue % (unsigned)base];
        uvalue = (uvalue / (unsigned)base);
    } while (uvalue && (place < (int)sizeof(convert)));
    if (place == sizeof(convert))
        place--;
    convert[place] = 0;

    zpadlen = max - place;
    spadlen =
        min - OSSL_MAX(max, place) - (signvalue ? 1 : 0) - strlen(prefix);
    if (zpadlen < 0)
        zpadlen = 0;
    if (spadlen < 0)
        spadlen = 0;
    if (flags & DP_F_ZERO) {
        zpadlen = OSSL_MAX(zpadlen, spadlen);
        spadlen = 0;
    }
    if (flags & DP_F_MINUS)
        spadlen = -spadlen;

    /* spaces */
    while (spadlen > 0) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, ' ');
        --spadlen;
    }

    /* sign */
    if (signvalue)
        doapr_outch(sbuffer, buffer, currlen, maxlen, signvalue);

    /* prefix */
    while (*prefix) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, *prefix);
        prefix++;
    }

    /* zeros */
    if (zpadlen > 0) {
        while (zpadlen > 0) {
            doapr_outch(sbuffer, buffer, currlen, maxlen, '0');
            --zpadlen;
        }
    }
    /* digits */
    while (place > 0)
        doapr_outch(sbuffer, buffer, currlen, maxlen, convert[--place]);

    /* left justified spaces */
    while (spadlen < 0) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, ' ');
        ++spadlen;
    }
    return;
}