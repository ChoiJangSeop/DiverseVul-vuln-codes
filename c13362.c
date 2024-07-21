static int pkey_gost2018_decrypt(EVP_PKEY_CTX *pctx, unsigned char *key,
                          size_t *key_len, const unsigned char *in,
                          size_t in_len)
{
    const unsigned char *p = in;
    struct gost_pmeth_data *data;
    EVP_PKEY *priv;
    PSKeyTransport_gost *pst = NULL;
    int ret = 0;
    unsigned char expkeys[64];
    EVP_PKEY *eph_key = NULL;
    int pkey_nid;
    int mac_nid = NID_undef;
    int iv_len = 0;

    if (!(data = EVP_PKEY_CTX_get_data(pctx)) ||
        !(priv = EVP_PKEY_CTX_get0_pkey(pctx))) {
       GOSTerr(GOST_F_PKEY_GOST2018_DECRYPT, GOST_R_ERROR_COMPUTING_EXPORT_KEYS);
       ret = 0;
       goto err;
    }
    pkey_nid = EVP_PKEY_base_id(priv);

    switch (data->cipher_nid) {
    case NID_magma_ctr:
        mac_nid = NID_magma_mac;
        iv_len = 4;
        break;
    case NID_grasshopper_ctr:
        mac_nid = NID_grasshopper_mac;
        iv_len = 8;
        break;
    default:
        GOSTerr(GOST_F_PKEY_GOST2018_DECRYPT, GOST_R_INVALID_CIPHER);
        return -1;
        break;
    }
    if (!key) {
        *key_len = 32;
        return 1;
    }

    pst = d2i_PSKeyTransport_gost(NULL, (const unsigned char **)&p, in_len);
    if (!pst) {
        GOSTerr(GOST_F_PKEY_GOST2018_DECRYPT,
                GOST_R_ERROR_PARSING_KEY_TRANSPORT_INFO);
        return -1;
    }

    eph_key = X509_PUBKEY_get(pst->ephem_key);
/*
 * TODO beldmit
   1.  Checks the next three conditions fulfilling and terminates the
   connection with fatal error if not.

   o  Q_eph is on the same curve as server public key;

   o  Q_eph is not equal to zero point;

   o  q * Q_eph is not equal to zero point.
*/
    if (eph_key == NULL) {
       GOSTerr(GOST_F_PKEY_GOST2018_DECRYPT,
               GOST_R_ERROR_COMPUTING_EXPORT_KEYS);
       ret = 0;
       goto err;
    }
  
    if (data->shared_ukm_size == 0 && pst->ukm != NULL) {
        if (EVP_PKEY_CTX_ctrl(pctx, -1, -1, EVP_PKEY_CTRL_SET_IV,
        ASN1_STRING_length(pst->ukm), (void *)ASN1_STRING_get0_data(pst->ukm)) < 0) {
            GOSTerr(GOST_F_PKEY_GOST2018_DECRYPT, GOST_R_UKM_NOT_SET);
            goto err;
        }
    }

    if (gost_keg(data->shared_ukm, pkey_nid,
                 EC_KEY_get0_public_key(EVP_PKEY_get0(eph_key)),
                 EVP_PKEY_get0(priv), expkeys) <= 0) {
        GOSTerr(GOST_F_PKEY_GOST2018_DECRYPT,
                GOST_R_ERROR_COMPUTING_EXPORT_KEYS);
        goto err;
    }

    if (gost_kimp15(ASN1_STRING_get0_data(pst->psexp),
                    ASN1_STRING_length(pst->psexp), data->cipher_nid,
                    expkeys + 32, mac_nid, expkeys + 0, data->shared_ukm + 24,
                    iv_len, key) <= 0) {
        GOSTerr(GOST_F_PKEY_GOST2018_DECRYPT, GOST_R_CANNOT_UNPACK_EPHEMERAL_KEY);
        goto err;
    }

    ret = 1;
 err:
    OPENSSL_cleanse(expkeys, sizeof(expkeys));
    EVP_PKEY_free(eph_key);
    PSKeyTransport_gost_free(pst);
    return ret;
}