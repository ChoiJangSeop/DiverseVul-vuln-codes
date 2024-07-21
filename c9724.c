split_der(asn1buf *buf, uint8_t *const *der, size_t len, taginfo *tag_out)
{
    krb5_error_code ret;
    const uint8_t *contents, *remainder;
    size_t clen, rlen;

    ret = get_tag(*der, len, tag_out, &contents, &clen, &remainder, &rlen);
    if (ret)
        return ret;
    if (rlen != 0)
        return ASN1_BAD_LENGTH;
    insert_bytes(buf, contents, clen);
    return 0;
}