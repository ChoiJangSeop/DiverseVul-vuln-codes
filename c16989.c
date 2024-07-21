BIO *PKCS7_dataDecode(PKCS7 *p7, EVP_PKEY *pkey, BIO *in_bio, X509 *pcert)
{
    int i, j;
    BIO *out = NULL, *btmp = NULL, *etmp = NULL, *bio = NULL;
    X509_ALGOR *xa;
    ASN1_OCTET_STRING *data_body = NULL;
    const EVP_MD *evp_md;
    const EVP_CIPHER *evp_cipher = NULL;
    EVP_CIPHER_CTX *evp_ctx = NULL;
    X509_ALGOR *enc_alg = NULL;
    STACK_OF(X509_ALGOR) *md_sk = NULL;
    STACK_OF(PKCS7_RECIP_INFO) *rsk = NULL;
    PKCS7_RECIP_INFO *ri = NULL;
    unsigned char *ek = NULL, *tkey = NULL;
    int eklen = 0, tkeylen = 0;

    if (p7 == NULL) {
        PKCS7err(PKCS7_F_PKCS7_DATADECODE, PKCS7_R_INVALID_NULL_POINTER);
        return NULL;
    }

    if (p7->d.ptr == NULL) {
        PKCS7err(PKCS7_F_PKCS7_DATADECODE, PKCS7_R_NO_CONTENT);
        return NULL;
    }

    i = OBJ_obj2nid(p7->type);
    p7->state = PKCS7_S_HEADER;

    switch (i) {
    case NID_pkcs7_signed:
        /*
         * p7->d.sign->contents is a PKCS7 structure consisting of a contentType
         * field and optional content.
         * data_body is NULL if that structure has no (=detached) content
         * or if the contentType is wrong (i.e., not "data").
         */
        data_body = PKCS7_get_octet_string(p7->d.sign->contents);
        if (!PKCS7_is_detached(p7) && data_body == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATADECODE,
                     PKCS7_R_INVALID_SIGNED_DATA_TYPE);
            goto err;
        }
        md_sk = p7->d.sign->md_algs;
        break;
    case NID_pkcs7_signedAndEnveloped:
        rsk = p7->d.signed_and_enveloped->recipientinfo;
        md_sk = p7->d.signed_and_enveloped->md_algs;
        /* data_body is NULL if the optional EncryptedContent is missing. */
        data_body = p7->d.signed_and_enveloped->enc_data->enc_data;
        enc_alg = p7->d.signed_and_enveloped->enc_data->algorithm;
        evp_cipher = EVP_get_cipherbyobj(enc_alg->algorithm);
        if (evp_cipher == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATADECODE,
                     PKCS7_R_UNSUPPORTED_CIPHER_TYPE);
            goto err;
        }
        break;
    case NID_pkcs7_enveloped:
        rsk = p7->d.enveloped->recipientinfo;
        enc_alg = p7->d.enveloped->enc_data->algorithm;
        /* data_body is NULL if the optional EncryptedContent is missing. */
        data_body = p7->d.enveloped->enc_data->enc_data;
        evp_cipher = EVP_get_cipherbyobj(enc_alg->algorithm);
        if (evp_cipher == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATADECODE,
                     PKCS7_R_UNSUPPORTED_CIPHER_TYPE);
            goto err;
        }
        break;
    default:
        PKCS7err(PKCS7_F_PKCS7_DATADECODE, PKCS7_R_UNSUPPORTED_CONTENT_TYPE);
        goto err;
    }

    /* Detached content must be supplied via in_bio instead. */
    if (data_body == NULL && in_bio == NULL) {
        PKCS7err(PKCS7_F_PKCS7_DATADECODE, PKCS7_R_NO_CONTENT);
        goto err;
    }

    /* We will be checking the signature */
    if (md_sk != NULL) {
        for (i = 0; i < sk_X509_ALGOR_num(md_sk); i++) {
            xa = sk_X509_ALGOR_value(md_sk, i);
            if ((btmp = BIO_new(BIO_f_md())) == NULL) {
                PKCS7err(PKCS7_F_PKCS7_DATADECODE, ERR_R_BIO_LIB);
                goto err;
            }

            j = OBJ_obj2nid(xa->algorithm);
            evp_md = EVP_get_digestbynid(j);
            if (evp_md == NULL) {
                PKCS7err(PKCS7_F_PKCS7_DATADECODE,
                         PKCS7_R_UNKNOWN_DIGEST_TYPE);
                goto err;
            }

            BIO_set_md(btmp, evp_md);
            if (out == NULL)
                out = btmp;
            else
                BIO_push(out, btmp);
            btmp = NULL;
        }
    }

    if (evp_cipher != NULL) {
        if ((etmp = BIO_new(BIO_f_cipher())) == NULL) {
            PKCS7err(PKCS7_F_PKCS7_DATADECODE, ERR_R_BIO_LIB);
            goto err;
        }

        /*
         * It was encrypted, we need to decrypt the secret key with the
         * private key
         */

        /*
         * Find the recipientInfo which matches the passed certificate (if
         * any)
         */

        if (pcert) {
            for (i = 0; i < sk_PKCS7_RECIP_INFO_num(rsk); i++) {
                ri = sk_PKCS7_RECIP_INFO_value(rsk, i);
                if (!pkcs7_cmp_ri(ri, pcert))
                    break;
                ri = NULL;
            }
            if (ri == NULL) {
                PKCS7err(PKCS7_F_PKCS7_DATADECODE,
                         PKCS7_R_NO_RECIPIENT_MATCHES_CERTIFICATE);
                goto err;
            }
        }

        /* If we haven't got a certificate try each ri in turn */
        if (pcert == NULL) {
            /*
             * Always attempt to decrypt all rinfo even after success as a
             * defence against MMA timing attacks.
             */
            for (i = 0; i < sk_PKCS7_RECIP_INFO_num(rsk); i++) {
                ri = sk_PKCS7_RECIP_INFO_value(rsk, i);

                if (pkcs7_decrypt_rinfo(&ek, &eklen, ri, pkey) < 0)
                    goto err;
                ERR_clear_error();
            }
        } else {
            /* Only exit on fatal errors, not decrypt failure */
            if (pkcs7_decrypt_rinfo(&ek, &eklen, ri, pkey) < 0)
                goto err;
            ERR_clear_error();
        }

        evp_ctx = NULL;
        BIO_get_cipher_ctx(etmp, &evp_ctx);
        if (EVP_CipherInit_ex(evp_ctx, evp_cipher, NULL, NULL, NULL, 0) <= 0)
            goto err;
        if (EVP_CIPHER_asn1_to_param(evp_ctx, enc_alg->parameter) < 0)
            goto err;
        /* Generate random key as MMA defence */
        tkeylen = EVP_CIPHER_CTX_key_length(evp_ctx);
        tkey = OPENSSL_malloc(tkeylen);
        if (tkey == NULL)
            goto err;
        if (EVP_CIPHER_CTX_rand_key(evp_ctx, tkey) <= 0)
            goto err;
        if (ek == NULL) {
            ek = tkey;
            eklen = tkeylen;
            tkey = NULL;
        }

        if (eklen != EVP_CIPHER_CTX_key_length(evp_ctx)) {
            /*
             * Some S/MIME clients don't use the same key and effective key
             * length. The key length is determined by the size of the
             * decrypted RSA key.
             */
            if (!EVP_CIPHER_CTX_set_key_length(evp_ctx, eklen)) {
                /* Use random key as MMA defence */
                OPENSSL_clear_free(ek, eklen);
                ek = tkey;
                eklen = tkeylen;
                tkey = NULL;
            }
        }
        /* Clear errors so we don't leak information useful in MMA */
        ERR_clear_error();
        if (EVP_CipherInit_ex(evp_ctx, NULL, NULL, ek, NULL, 0) <= 0)
            goto err;

        OPENSSL_clear_free(ek, eklen);
        ek = NULL;
        OPENSSL_clear_free(tkey, tkeylen);
        tkey = NULL;

        if (out == NULL)
            out = etmp;
        else
            BIO_push(out, etmp);
        etmp = NULL;
    }
    if (in_bio != NULL) {
        bio = in_bio;
    } else {
        if (data_body->length > 0)
            bio = BIO_new_mem_buf(data_body->data, data_body->length);
        else {
            bio = BIO_new(BIO_s_mem());
            if (bio == NULL)
                goto err;
            BIO_set_mem_eof_return(bio, 0);
        }
        if (bio == NULL)
            goto err;
    }
    BIO_push(out, bio);
    bio = NULL;
    return out;

 err:
    OPENSSL_clear_free(ek, eklen);
    OPENSSL_clear_free(tkey, tkeylen);
    BIO_free_all(out);
    BIO_free_all(btmp);
    BIO_free_all(etmp);
    BIO_free_all(bio);
    return NULL;
}