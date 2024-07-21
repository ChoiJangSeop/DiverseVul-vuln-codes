static int pkey_GOST_ECcp_encrypt(EVP_PKEY_CTX *pctx, unsigned char *out,
                           size_t *out_len, const unsigned char *key,
                           size_t key_len)
{
    GOST_KEY_TRANSPORT *gkt = NULL;
    EVP_PKEY *pubk = EVP_PKEY_CTX_get0_pkey(pctx);
    struct gost_pmeth_data *data = EVP_PKEY_CTX_get_data(pctx);
    int pkey_nid = EVP_PKEY_base_id(pubk);
    ASN1_OBJECT *crypt_params_obj = (pkey_nid == NID_id_GostR3410_2001 || pkey_nid == NID_id_GostR3410_2001DH) ?
        OBJ_nid2obj(NID_id_Gost28147_89_CryptoPro_A_ParamSet) :
        OBJ_nid2obj(NID_id_tc26_gost_28147_param_Z);
    const struct gost_cipher_info *param =
        get_encryption_params(crypt_params_obj);
    unsigned char ukm[8], shared_key[32], crypted_key[44];
    int ret = 0;
    int key_is_ephemeral = 1;
    gost_ctx cctx;
    EVP_PKEY *sec_key = EVP_PKEY_CTX_get0_peerkey(pctx);
    if (data->shared_ukm_size) {
        memcpy(ukm, data->shared_ukm, 8);
    } else {
        if (RAND_bytes(ukm, 8) <= 0) {
            GOSTerr(GOST_F_PKEY_GOST_ECCP_ENCRYPT, GOST_R_RNG_ERROR);
            return 0;
        }
    }
    if (!param)
        goto err;
    /* Check for private key in the peer_key of context */
    if (sec_key) {
        key_is_ephemeral = 0;
        if (!gost_get0_priv_key(sec_key)) {
            GOSTerr(GOST_F_PKEY_GOST_ECCP_ENCRYPT,
                    GOST_R_NO_PRIVATE_PART_OF_NON_EPHEMERAL_KEYPAIR);
            goto err;
        }
    } else {
        key_is_ephemeral = 1;
        if (out) {
            sec_key = EVP_PKEY_new();
            if (!EVP_PKEY_assign(sec_key, EVP_PKEY_base_id(pubk), EC_KEY_new())
                || !EVP_PKEY_copy_parameters(sec_key, pubk)
                || !gost_ec_keygen(EVP_PKEY_get0(sec_key))) {
                GOSTerr(GOST_F_PKEY_GOST_ECCP_ENCRYPT,
                        GOST_R_ERROR_COMPUTING_SHARED_KEY);
                goto err;
            }
        }
    }
    if (out) {
        int dgst_nid = NID_undef;
        EVP_PKEY_get_default_digest_nid(pubk, &dgst_nid);
        if (dgst_nid == NID_id_GostR3411_2012_512)
            dgst_nid = NID_id_GostR3411_2012_256;

        if (!VKO_compute_key(shared_key,
                             EC_KEY_get0_public_key(EVP_PKEY_get0(pubk)),
                             EVP_PKEY_get0(sec_key), ukm, 8, dgst_nid)) {
            GOSTerr(GOST_F_PKEY_GOST_ECCP_ENCRYPT,
                    GOST_R_ERROR_COMPUTING_SHARED_KEY);
            goto err;
        }
        gost_init(&cctx, param->sblock);
        keyWrapCryptoPro(&cctx, shared_key, ukm, key, crypted_key);
    }
    gkt = GOST_KEY_TRANSPORT_new();
    if (!gkt) {
        goto err;
    }
    if (!ASN1_OCTET_STRING_set(gkt->key_agreement_info->eph_iv, ukm, 8)) {
        goto err;
    }
    if (!ASN1_OCTET_STRING_set(gkt->key_info->imit, crypted_key + 40, 4)) {
        goto err;
    }
    if (!ASN1_OCTET_STRING_set
        (gkt->key_info->encrypted_key, crypted_key + 8, 32)) {
        goto err;
    }
    if (key_is_ephemeral) {
        if (!X509_PUBKEY_set
            (&gkt->key_agreement_info->ephem_key, out ? sec_key : pubk)) {
            GOSTerr(GOST_F_PKEY_GOST_ECCP_ENCRYPT,
                    GOST_R_CANNOT_PACK_EPHEMERAL_KEY);
            goto err;
        }
    }
    ASN1_OBJECT_free(gkt->key_agreement_info->cipher);
    gkt->key_agreement_info->cipher = OBJ_nid2obj(param->nid);
    if (key_is_ephemeral)
        EVP_PKEY_free(sec_key);
    if (!key_is_ephemeral) {
        /* Set control "public key from client certificate used" */
        if (EVP_PKEY_CTX_ctrl(pctx, -1, -1, EVP_PKEY_CTRL_PEER_KEY, 3, NULL)
            <= 0) {
            GOSTerr(GOST_F_PKEY_GOST_ECCP_ENCRYPT, GOST_R_CTRL_CALL_FAILED);
            goto err;
        }
    }
    if ((*out_len = i2d_GOST_KEY_TRANSPORT(gkt, out ? &out : NULL)) > 0)
        ret = 1;
    OPENSSL_cleanse(shared_key, sizeof(shared_key));
    GOST_KEY_TRANSPORT_free(gkt);
    return ret;
 err:
    OPENSSL_cleanse(shared_key, sizeof(shared_key));
    if (key_is_ephemeral)
        EVP_PKEY_free(sec_key);
    GOST_KEY_TRANSPORT_free(gkt);
    return -1;
}