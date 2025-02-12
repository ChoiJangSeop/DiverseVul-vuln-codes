int PKCS7_dataFinal(PKCS7 *p7, BIO *bio)
{
    int ret = 0;
    int i, j;
    BIO *btmp;
    BUF_MEM *buf_mem = NULL;
    BUF_MEM *buf = NULL;
    PKCS7_SIGNER_INFO *si;
    EVP_MD_CTX *mdc, ctx_tmp;
    STACK_OF(X509_ATTRIBUTE) *sk;
    STACK_OF(PKCS7_SIGNER_INFO) *si_sk = NULL;
    ASN1_OCTET_STRING *os = NULL;

    EVP_MD_CTX_init(&ctx_tmp);
    i = OBJ_obj2nid(p7->type);
    p7->state = PKCS7_S_HEADER;

    switch (i) {
    case NID_pkcs7_signedAndEnveloped:
        /* XXXXXXXXXXXXXXXX */
        si_sk = p7->d.signed_and_enveloped->signer_info;
        if (!(os = M_ASN1_OCTET_STRING_new())) {
            PKCS7err(PKCS7_F_PKCS7_DATAFINAL, ERR_R_MALLOC_FAILURE);
            goto err;
        }
        p7->d.signed_and_enveloped->enc_data->enc_data = os;
        break;
    case NID_pkcs7_enveloped:
        /* XXXXXXXXXXXXXXXX */
        if (!(os = M_ASN1_OCTET_STRING_new())) {
            PKCS7err(PKCS7_F_PKCS7_DATAFINAL, ERR_R_MALLOC_FAILURE);
            goto err;
        }
        p7->d.enveloped->enc_data->enc_data = os;
        break;
    case NID_pkcs7_signed:
        si_sk = p7->d.sign->signer_info;
        os = PKCS7_get_octet_string(p7->d.sign->contents);
        /* If detached data then the content is excluded */
        if (PKCS7_type_is_data(p7->d.sign->contents) && p7->detached) {
            M_ASN1_OCTET_STRING_free(os);
            p7->d.sign->contents->d.data = NULL;
        }
        break;

    case NID_pkcs7_digest:
        os = PKCS7_get_octet_string(p7->d.digest->contents);
        /* If detached data then the content is excluded */
        if (PKCS7_type_is_data(p7->d.digest->contents) && p7->detached) {
            M_ASN1_OCTET_STRING_free(os);
            p7->d.digest->contents->d.data = NULL;
        }
        break;

    }

    if (si_sk != NULL) {
        if ((buf = BUF_MEM_new()) == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATAFINAL, ERR_R_BIO_LIB);
            goto err;
        }
        for (i = 0; i < sk_PKCS7_SIGNER_INFO_num(si_sk); i++) {
            si = sk_PKCS7_SIGNER_INFO_value(si_sk, i);
            if (si->pkey == NULL)
                continue;

            j = OBJ_obj2nid(si->digest_alg->algorithm);

            btmp = bio;

            btmp = PKCS7_find_digest(&mdc, btmp, j);

            if (btmp == NULL)
                goto err;

            /*
             * We now have the EVP_MD_CTX, lets do the signing.
             */
            EVP_MD_CTX_copy_ex(&ctx_tmp, mdc);
            if (!BUF_MEM_grow_clean(buf, EVP_PKEY_size(si->pkey))) {
                PKCS7err(PKCS7_F_PKCS7_DATAFINAL, ERR_R_BIO_LIB);
                goto err;
            }

            sk = si->auth_attr;

            /*
             * If there are attributes, we add the digest attribute and only
             * sign the attributes
             */
            if ((sk != NULL) && (sk_X509_ATTRIBUTE_num(sk) != 0)) {
                unsigned char md_data[EVP_MAX_MD_SIZE], *abuf = NULL;
                unsigned int md_len, alen;
                ASN1_OCTET_STRING *digest;
                ASN1_UTCTIME *sign_time;
                const EVP_MD *md_tmp;

                /* Add signing time if not already present */
                if (!PKCS7_get_signed_attribute(si, NID_pkcs9_signingTime)) {
                    if (!(sign_time = X509_gmtime_adj(NULL, 0))) {
                        PKCS7err(PKCS7_F_PKCS7_DATAFINAL,
                                 ERR_R_MALLOC_FAILURE);
                        goto err;
                    }
                    if (!PKCS7_add_signed_attribute(si,
                                                    NID_pkcs9_signingTime,
                                                    V_ASN1_UTCTIME,
                                                    sign_time)) {
                        M_ASN1_UTCTIME_free(sign_time);
                        goto err;
                    }
                }

                /* Add digest */
                md_tmp = EVP_MD_CTX_md(&ctx_tmp);
                EVP_DigestFinal_ex(&ctx_tmp, md_data, &md_len);
                if (!(digest = M_ASN1_OCTET_STRING_new())) {
                    PKCS7err(PKCS7_F_PKCS7_DATAFINAL, ERR_R_MALLOC_FAILURE);
                    goto err;
                }
                if (!M_ASN1_OCTET_STRING_set(digest, md_data, md_len)) {
                    PKCS7err(PKCS7_F_PKCS7_DATAFINAL, ERR_R_MALLOC_FAILURE);
                    M_ASN1_OCTET_STRING_free(digest);
                    goto err;
                }
                if (!PKCS7_add_signed_attribute(si,
                                                NID_pkcs9_messageDigest,
                                                V_ASN1_OCTET_STRING, digest))
                {
                    M_ASN1_OCTET_STRING_free(digest);
                    goto err;
                }

                /* Now sign the attributes */
                EVP_SignInit_ex(&ctx_tmp, md_tmp, NULL);
                alen = ASN1_item_i2d((ASN1_VALUE *)sk, &abuf,
                                     ASN1_ITEM_rptr(PKCS7_ATTR_SIGN));
                if (!abuf)
                    goto err;
                EVP_SignUpdate(&ctx_tmp, abuf, alen);
                OPENSSL_free(abuf);
            }
#ifndef OPENSSL_NO_DSA
            if (si->pkey->type == EVP_PKEY_DSA)
                ctx_tmp.digest = EVP_dss1();
#endif
#ifndef OPENSSL_NO_ECDSA
            if (si->pkey->type == EVP_PKEY_EC)
                ctx_tmp.digest = EVP_ecdsa();
#endif

            if (!EVP_SignFinal(&ctx_tmp, (unsigned char *)buf->data,
                               (unsigned int *)&buf->length, si->pkey)) {
                PKCS7err(PKCS7_F_PKCS7_DATAFINAL, ERR_R_EVP_LIB);
                goto err;
            }
            if (!ASN1_STRING_set(si->enc_digest,
                                 (unsigned char *)buf->data, buf->length)) {
                PKCS7err(PKCS7_F_PKCS7_DATAFINAL, ERR_R_ASN1_LIB);
                goto err;
            }
        }
    } else if (i == NID_pkcs7_digest) {
        unsigned char md_data[EVP_MAX_MD_SIZE];
        unsigned int md_len;
        if (!PKCS7_find_digest(&mdc, bio,
                               OBJ_obj2nid(p7->d.digest->md->algorithm)))
            goto err;
        EVP_DigestFinal_ex(mdc, md_data, &md_len);
        M_ASN1_OCTET_STRING_set(p7->d.digest->digest, md_data, md_len);
    }

    if (!PKCS7_is_detached(p7)) {
        btmp = BIO_find_type(bio, BIO_TYPE_MEM);
        if (btmp == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATAFINAL, PKCS7_R_UNABLE_TO_FIND_MEM_BIO);
            goto err;
        }
        BIO_get_mem_ptr(btmp, &buf_mem);
        /*
         * Mark the BIO read only then we can use its copy of the data
         * instead of making an extra copy.
         */
        BIO_set_flags(btmp, BIO_FLAGS_MEM_RDONLY);
        BIO_set_mem_eof_return(btmp, 0);
        os->data = (unsigned char *)buf_mem->data;
        os->length = buf_mem->length;
#if 0
        M_ASN1_OCTET_STRING_set(os,
                                (unsigned char *)buf_mem->data,
                                buf_mem->length);
#endif
    }
    ret = 1;
 err:
    EVP_MD_CTX_cleanup(&ctx_tmp);
    if (buf != NULL)
        BUF_MEM_free(buf);
    return (ret);
}