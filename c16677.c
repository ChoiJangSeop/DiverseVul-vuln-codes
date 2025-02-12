int i2c_ASN1_INTEGER(ASN1_INTEGER *a, unsigned char **pp)
{
    int pad = 0, ret, i, neg;
    unsigned char *p, *n, pb = 0;

    if (a == NULL)
        return (0);
    neg = a->type & V_ASN1_NEG;
    if (a->length == 0)
        ret = 1;
    else {
        ret = a->length;
        i = a->data[0];
        if (!neg && (i > 127)) {
            pad = 1;
            pb = 0;
        } else if (neg) {
            if (i > 128) {
                pad = 1;
                pb = 0xFF;
            } else if (i == 128) {
                /*
                 * Special case: if any other bytes non zero we pad:
                 * otherwise we don't.
                 */
                for (i = 1; i < a->length; i++)
                    if (a->data[i]) {
                        pad = 1;
                        pb = 0xFF;
                        break;
                    }
            }
        }
        ret += pad;
    }
    if (pp == NULL)
        return (ret);
    p = *pp;

    if (pad)
        *(p++) = pb;
    if (a->length == 0)
        *(p++) = 0;
    else if (!neg)
        memcpy(p, a->data, (unsigned int)a->length);
    else {
        /* Begin at the end of the encoding */
        n = a->data + a->length - 1;
        p += a->length - 1;
        i = a->length;
        /* Copy zeros to destination as long as source is zero */
        while (!*n) {
            *(p--) = 0;
            n--;
            i--;
        }
        /* Complement and increment next octet */
        *(p--) = ((*(n--)) ^ 0xff) + 1;
        i--;
        /* Complement any octets left */
        for (; i > 0; i--)
            *(p--) = *(n--) ^ 0xff;
    }

    *pp += ret;
    return (ret);
}