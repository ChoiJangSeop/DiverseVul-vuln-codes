int ASN1_item_ex_d2i(ASN1_VALUE **pval, const unsigned char **in, long len,
                     const ASN1_ITEM *it,
                     int tag, int aclass, char opt, ASN1_TLC *ctx)
{
    int rv;
    rv = asn1_item_embed_d2i(pval, in, len, it, tag, aclass, opt, ctx);
    if (rv <= 0)
        ASN1_item_ex_free(pval, it);
    return rv;
}