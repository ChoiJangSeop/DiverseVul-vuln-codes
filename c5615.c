get_matching_data(krb5_context context,
                  pkinit_plg_crypto_context plg_cryptoctx,
                  pkinit_req_crypto_context req_cryptoctx, X509 *cert,
                  pkinit_cert_matching_data **md_out)
{
    krb5_error_code ret = ENOMEM;
    pkinit_cert_matching_data *md = NULL;
    krb5_principal *pkinit_sans = NULL, *upn_sans = NULL;
    size_t i, j;
    char buf[DN_BUF_LEN];
    unsigned int bufsize = sizeof(buf);

    *md_out = NULL;

    md = calloc(1, sizeof(*md));
    if (md == NULL)
        goto cleanup;

    /* Get the subject name (in rfc2253 format). */
    X509_NAME_oneline_ex(X509_get_subject_name(cert), buf, &bufsize,
                         XN_FLAG_SEP_COMMA_PLUS);
    md->subject_dn = strdup(buf);
    if (md->subject_dn == NULL) {
        ret = ENOMEM;
        goto cleanup;
    }

    /* Get the issuer name (in rfc2253 format). */
    X509_NAME_oneline_ex(X509_get_issuer_name(cert), buf, &bufsize,
                         XN_FLAG_SEP_COMMA_PLUS);
    md->issuer_dn = strdup(buf);
    if (md->issuer_dn == NULL) {
        ret = ENOMEM;
        goto cleanup;
    }

    /* Get the SAN data. */
    ret = crypto_retrieve_X509_sans(context, plg_cryptoctx, req_cryptoctx,
                                    cert, &pkinit_sans, &upn_sans, NULL);
    if (ret)
        goto cleanup;

    j = 0;
    if (pkinit_sans != NULL) {
        for (i = 0; pkinit_sans[i] != NULL; i++)
            j++;
    }
    if (upn_sans != NULL) {
        for (i = 0; upn_sans[i] != NULL; i++)
            j++;
    }
    if (j != 0) {
        md->sans = calloc((size_t)j+1, sizeof(*md->sans));
        if (md->sans == NULL) {
            ret = ENOMEM;
            goto cleanup;
        }
        j = 0;
        if (pkinit_sans != NULL) {
            for (i = 0; pkinit_sans[i] != NULL; i++)
                md->sans[j++] = pkinit_sans[i];
            free(pkinit_sans);
        }
        if (upn_sans != NULL) {
            for (i = 0; upn_sans[i] != NULL; i++)
                md->sans[j++] = upn_sans[i];
            free(upn_sans);
        }
        md->sans[j] = NULL;
    } else
        md->sans = NULL;

    /* Get the KU and EKU data. */
    ret = crypto_retrieve_X509_key_usage(context, plg_cryptoctx,
                                         req_cryptoctx, cert, &md->ku_bits,
                                         &md->eku_bits);
    if (ret)
        goto cleanup;

    *md_out = md;
    md = NULL;

cleanup:
    crypto_cert_free_matching_data(context, md);
    return ret;
}