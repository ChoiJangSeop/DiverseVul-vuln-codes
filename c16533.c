EC_KEY *d2i_ECPrivateKey(EC_KEY **a, const unsigned char **in, long len)
{
    int ok = 0;
    EC_KEY *ret = NULL;
    EC_PRIVATEKEY *priv_key = NULL;

    if ((priv_key = EC_PRIVATEKEY_new()) == NULL) {
        ECerr(EC_F_D2I_ECPRIVATEKEY, ERR_R_MALLOC_FAILURE);
        return NULL;
    }

    if ((priv_key = d2i_EC_PRIVATEKEY(&priv_key, in, len)) == NULL) {
        ECerr(EC_F_D2I_ECPRIVATEKEY, ERR_R_EC_LIB);
        EC_PRIVATEKEY_free(priv_key);
        return NULL;
    }

    if (a == NULL || *a == NULL) {
        if ((ret = EC_KEY_new()) == NULL) {
            ECerr(EC_F_D2I_ECPRIVATEKEY, ERR_R_MALLOC_FAILURE);
            goto err;
        }
        if (a)
            *a = ret;
    } else
        ret = *a;

    if (priv_key->parameters) {
        if (ret->group)
            EC_GROUP_clear_free(ret->group);
        ret->group = ec_asn1_pkparameters2group(priv_key->parameters);
    }

    if (ret->group == NULL) {
        ECerr(EC_F_D2I_ECPRIVATEKEY, ERR_R_EC_LIB);
        goto err;
    }

    ret->version = priv_key->version;

    if (priv_key->privateKey) {
        ret->priv_key = BN_bin2bn(M_ASN1_STRING_data(priv_key->privateKey),
                                  M_ASN1_STRING_length(priv_key->privateKey),
                                  ret->priv_key);
        if (ret->priv_key == NULL) {
            ECerr(EC_F_D2I_ECPRIVATEKEY, ERR_R_BN_LIB);
            goto err;
        }
    } else {
        ECerr(EC_F_D2I_ECPRIVATEKEY, EC_R_MISSING_PRIVATE_KEY);
        goto err;
    }

    if (priv_key->publicKey) {
        const unsigned char *pub_oct;
        size_t pub_oct_len;

        if (ret->pub_key)
            EC_POINT_clear_free(ret->pub_key);
        ret->pub_key = EC_POINT_new(ret->group);
        if (ret->pub_key == NULL) {
            ECerr(EC_F_D2I_ECPRIVATEKEY, ERR_R_EC_LIB);
            goto err;
        }
        pub_oct = M_ASN1_STRING_data(priv_key->publicKey);
        pub_oct_len = M_ASN1_STRING_length(priv_key->publicKey);
        /* save the point conversion form */
        ret->conv_form = (point_conversion_form_t) (pub_oct[0] & ~0x01);
        if (!EC_POINT_oct2point(ret->group, ret->pub_key,
                                pub_oct, pub_oct_len, NULL)) {
            ECerr(EC_F_D2I_ECPRIVATEKEY, ERR_R_EC_LIB);
            goto err;
        }
    }

    ok = 1;
 err:
    if (!ok) {
        if (ret)
            EC_KEY_free(ret);
        ret = NULL;
    }

    if (priv_key)
        EC_PRIVATEKEY_free(priv_key);

    return (ret);
}