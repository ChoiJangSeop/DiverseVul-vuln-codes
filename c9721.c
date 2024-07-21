k5_asn1_full_decode(const krb5_data *code, const struct atype_info *a,
                    void **retrep)
{
    krb5_error_code ret;
    const uint8_t *contents, *remainder;
    size_t clen, rlen;
    taginfo t;

    *retrep = NULL;
    ret = get_tag((uint8_t *)code->data, code->length, &t, &contents,
                  &clen, &remainder, &rlen);
    if (ret)
        return ret;
    /* rlen should be 0, but we don't check it (and due to padding in
     * non-length-preserving enctypes, it will sometimes be nonzero). */
    if (!check_atype_tag(a, &t))
        return ASN1_BAD_ID;
    return decode_atype_to_ptr(&t, contents, clen, a, retrep);
}