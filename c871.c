krb5_verify_init_creds(krb5_context context, krb5_creds *creds,
                       krb5_principal server, krb5_keytab keytab,
                       krb5_ccache *ccache,
                       krb5_verify_init_creds_opt *options)
{
    krb5_error_code ret;
    krb5_principal *host_princs = NULL;
    krb5_keytab defkeytab = NULL;
    krb5_keytab_entry kte;
    krb5_boolean have_keys = FALSE;
    size_t i;

    if (keytab == NULL) {
        ret = krb5_kt_default(context, &defkeytab);
        if (ret)
            goto cleanup;
        keytab = defkeytab;
    }

    if (server != NULL) {
        /* Check if server exists in keytab first. */
        ret = krb5_kt_get_entry(context, keytab, server, 0, 0, &kte);
        if (ret)
            goto cleanup;
        krb5_kt_free_entry(context, &kte);
        have_keys = TRUE;
        ret = get_vfy_cred(context, creds, server, keytab, ccache);
    } else {
        /* Try using the host service principals from the keytab. */
        if (keytab->ops->start_seq_get == NULL) {
            ret = EINVAL;
            goto cleanup;
        }
        ret = get_host_princs_from_keytab(context, keytab, &host_princs);
        if (ret)
            goto cleanup;
        if (host_princs == NULL) {
            ret = KRB5_KT_NOTFOUND;
            goto cleanup;
        }
        have_keys = TRUE;

        /* Try all host principals until one succeeds or they all fail. */
        for (i = 0; host_princs[i] != NULL; i++) {
            ret = get_vfy_cred(context, creds, host_princs[i], keytab, ccache);
            if (ret == 0)
                break;
        }
    }

cleanup:
    /* If we have no key to verify with, pretend to succeed unless
     * configuration directs otherwise. */
    if (!have_keys && !nofail(context, options, creds))
        ret = 0;

    if (defkeytab != NULL)
        krb5_kt_close(context, defkeytab);
    krb5_free_principal(context, server);
    free_princ_list(context, host_princs);

    return ret;
}