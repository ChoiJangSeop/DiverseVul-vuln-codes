int pkey_gost_decrypt(EVP_PKEY_CTX *pctx, unsigned char *key,
                      size_t *key_len, const unsigned char *in, size_t in_len)
{
    struct gost_pmeth_data *gctx = EVP_PKEY_CTX_get_data(pctx);
    switch (gctx->cipher_nid)
    {
        case NID_id_Gost28147_89:
        case NID_undef: /* FIXME */
            return pkey_GOST_ECcp_decrypt(pctx, key, key_len, in, in_len);
        case NID_kuznyechik_ctr:
        case NID_magma_ctr:
            return pkey_gost2018_decrypt(pctx, key, key_len, in, in_len);
        default:
      GOSTerr(GOST_F_PKEY_GOST_DECRYPT, ERR_R_INTERNAL_ERROR);
      return -1;
    }
}