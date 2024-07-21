get_tag(const uint8_t *asn1, size_t len, taginfo *tag_out,
        const uint8_t **contents_out, size_t *clen_out,
        const uint8_t **remainder_out, size_t *rlen_out)
{
    krb5_error_code ret;
    uint8_t o;
    const uint8_t *c, *p, *tag_start = asn1;
    size_t clen, llen, i;
    taginfo t;

    *contents_out = *remainder_out = NULL;
    *clen_out = *rlen_out = 0;
    if (len == 0)
        return ASN1_OVERRUN;
    o = *asn1++;
    len--;
    tag_out->asn1class = o & 0xC0;
    tag_out->construction = o & 0x20;
    if ((o & 0x1F) != 0x1F) {
        tag_out->tagnum = o & 0x1F;
    } else {
        tag_out->tagnum = 0;
        do {
            if (len == 0)
                return ASN1_OVERRUN;
            o = *asn1++;
            len--;
            tag_out->tagnum = (tag_out->tagnum << 7) | (o & 0x7F);
        } while (o & 0x80);
    }

    if (len == 0)
        return ASN1_OVERRUN;
    o = *asn1++;
    len--;

    if (o == 0x80) {
        /* Indefinite form (should not be present in DER, but we accept it). */
        if (tag_out->construction != CONSTRUCTED)
            return ASN1_MISMATCH_INDEF;
        p = asn1;
        while (!(len >= 2 && p[0] == 0 && p[1] == 0)) {
            ret = get_tag(p, len, &t, &c, &clen, &p, &len);
            if (ret)
                return ret;
        }
        tag_out->tag_end_len = 2;
        *contents_out = asn1;
        *clen_out = p - asn1;
        *remainder_out = p + 2;
        *rlen_out = len - 2;
    } else if ((o & 0x80) == 0) {
        /* Short form (first octet gives content length). */
        if (o > len)
            return ASN1_OVERRUN;
        tag_out->tag_end_len = 0;
        *contents_out = asn1;
        *clen_out = o;
        *remainder_out = asn1 + *clen_out;
        *rlen_out = len - (*remainder_out - asn1);
    } else {
        /* Long form (first octet gives number of base-256 length octets). */
        llen = o & 0x7F;
        if (llen > len)
            return ASN1_OVERRUN;
        if (llen > sizeof(*clen_out))
            return ASN1_OVERFLOW;
        for (i = 0, clen = 0; i < llen; i++)
            clen = (clen << 8) | asn1[i];
        if (clen > len - llen)
            return ASN1_OVERRUN;
        tag_out->tag_end_len = 0;
        *contents_out = asn1 + llen;
        *clen_out = clen;
        *remainder_out = *contents_out + clen;
        *rlen_out = len - (*remainder_out - asn1);
    }
    tag_out->tag_len = *contents_out - tag_start;
    return 0;
}