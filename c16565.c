BIO *PKCS7_dataInit(PKCS7 *p7, BIO *bio)
{
    int i;
    BIO *out = NULL, *btmp = NULL;
    X509_ALGOR *xa = NULL;
    const EVP_CIPHER *evp_cipher = NULL;
    STACK_OF(X509_ALGOR) *md_sk = NULL;
    STACK_OF(PKCS7_RECIP_INFO) *rsk = NULL;
    X509_ALGOR *xalg = NULL;
    PKCS7_RECIP_INFO *ri = NULL;
    EVP_PKEY *pkey;
    ASN1_OCTET_STRING *os = NULL;

    i = OBJ_obj2nid(p7->type);
    p7->state = PKCS7_S_HEADER;

    switch (i) {
    case NID_pkcs7_signed:
        md_sk = p7->d.sign->md_algs;
        os = PKCS7_get_octet_string(p7->d.sign->contents);
        break;
    case NID_pkcs7_signedAndEnveloped:
        rsk = p7->d.signed_and_enveloped->recipientinfo;
        md_sk = p7->d.signed_and_enveloped->md_algs;
        xalg = p7->d.signed_and_enveloped->enc_data->algorithm;
        evp_cipher = p7->d.signed_and_enveloped->enc_data->cipher;
        if (evp_cipher == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATAINIT, PKCS7_R_CIPHER_NOT_INITIALIZED);
            goto err;
        }
        break;
    case NID_pkcs7_enveloped:
        rsk = p7->d.enveloped->recipientinfo;
        xalg = p7->d.enveloped->enc_data->algorithm;
        evp_cipher = p7->d.enveloped->enc_data->cipher;
        if (evp_cipher == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATAINIT, PKCS7_R_CIPHER_NOT_INITIALIZED);
            goto err;
        }
        break;
    case NID_pkcs7_digest:
        xa = p7->d.digest->md;
        os = PKCS7_get_octet_string(p7->d.digest->contents);
        break;
    default:
        PKCS7err(PKCS7_F_PKCS7_DATAINIT, PKCS7_R_UNSUPPORTED_CONTENT_TYPE);
        goto err;
    }

    for (i = 0; i < sk_X509_ALGOR_num(md_sk); i++)
        if (!PKCS7_bio_add_digest(&out, sk_X509_ALGOR_value(md_sk, i)))
            goto err;

    if (xa && !PKCS7_bio_add_digest(&out, xa))
        goto err;

    if (evp_cipher != NULL) {
        unsigned char key[EVP_MAX_KEY_LENGTH];
        unsigned char iv[EVP_MAX_IV_LENGTH];
        int keylen, ivlen;
        int jj, max;
        unsigned char *tmp;
        EVP_CIPHER_CTX *ctx;

        if ((btmp = BIO_new(BIO_f_cipher())) == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATAINIT, ERR_R_BIO_LIB);
            goto err;
        }
        BIO_get_cipher_ctx(btmp, &ctx);
        keylen = EVP_CIPHER_key_length(evp_cipher);
        ivlen = EVP_CIPHER_iv_length(evp_cipher);
        xalg->algorithm = OBJ_nid2obj(EVP_CIPHER_type(evp_cipher));
        if (ivlen > 0)
            if (RAND_pseudo_bytes(iv, ivlen) <= 0)
                goto err;
        if (EVP_CipherInit_ex(ctx, evp_cipher, NULL, NULL, NULL, 1) <= 0)
            goto err;
        if (EVP_CIPHER_CTX_rand_key(ctx, key) <= 0)
            goto err;
        if (EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, 1) <= 0)
            goto err;

        if (ivlen > 0) {
            if (xalg->parameter == NULL) {
                xalg->parameter = ASN1_TYPE_new();
                if (xalg->parameter == NULL)
                    goto err;
            }
            if (EVP_CIPHER_param_to_asn1(ctx, xalg->parameter) < 0)
                goto err;
        }

        /* Lets do the pub key stuff :-) */
        max = 0;
        for (i = 0; i < sk_PKCS7_RECIP_INFO_num(rsk); i++) {
            ri = sk_PKCS7_RECIP_INFO_value(rsk, i);
            if (ri->cert == NULL) {
                PKCS7err(PKCS7_F_PKCS7_DATAINIT,
                         PKCS7_R_MISSING_CERIPEND_INFO);
                goto err;
            }
            if ((pkey = X509_get_pubkey(ri->cert)) == NULL)
                goto err;
            jj = EVP_PKEY_size(pkey);
            EVP_PKEY_free(pkey);
            if (max < jj)
                max = jj;
        }
        if ((tmp = (unsigned char *)OPENSSL_malloc(max)) == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATAINIT, ERR_R_MALLOC_FAILURE);
            goto err;
        }
        for (i = 0; i < sk_PKCS7_RECIP_INFO_num(rsk); i++) {
            ri = sk_PKCS7_RECIP_INFO_value(rsk, i);
            if ((pkey = X509_get_pubkey(ri->cert)) == NULL)
                goto err;
            jj = EVP_PKEY_encrypt(tmp, key, keylen, pkey);
            EVP_PKEY_free(pkey);
            if (jj <= 0) {
                PKCS7err(PKCS7_F_PKCS7_DATAINIT, ERR_R_EVP_LIB);
                OPENSSL_free(tmp);
                goto err;
            }
            if (!M_ASN1_OCTET_STRING_set(ri->enc_key, tmp, jj)) {
                PKCS7err(PKCS7_F_PKCS7_DATAINIT, ERR_R_MALLOC_FAILURE);
                OPENSSL_free(tmp);
                goto err;
            }
        }
        OPENSSL_free(tmp);
        OPENSSL_cleanse(key, keylen);

        if (out == NULL)
            out = btmp;
        else
            BIO_push(out, btmp);
        btmp = NULL;
    }

    if (bio == NULL) {
        if (PKCS7_is_detached(p7))
            bio = BIO_new(BIO_s_null());
        else if (os && os->length > 0)
            bio = BIO_new_mem_buf(os->data, os->length);
        if (bio == NULL) {
            bio = BIO_new(BIO_s_mem());
            if (bio == NULL)
                goto err;
            BIO_set_mem_eof_return(bio, 0);
        }
    }
    BIO_push(out, bio);
    bio = NULL;
    if (0) {
 err:
        if (out != NULL)
            BIO_free_all(out);
        if (btmp != NULL)
            BIO_free_all(btmp);
        out = NULL;
    }
    return (out);
}