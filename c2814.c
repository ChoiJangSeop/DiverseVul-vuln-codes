static krb5_error_code verify_krb5_user(krb5_context context, krb5_principal principal, const char *password, krb5_principal server)
{
    krb5_creds creds;
    krb5_get_init_creds_opt gic_options;
    krb5_error_code ret;
    char *name = NULL;

    memset(&creds, 0, sizeof(creds));

    ret = krb5_unparse_name(context, principal, &name);
    if (ret == 0)
    {
#ifdef PRINTFS
        printf("Trying to get TGT for user %s\n", name);
#endif
        free(name);
    }

    krb5_get_init_creds_opt_init(&gic_options);
    ret = krb5_get_init_creds_password(context, &creds, principal, (char *)password, NULL, NULL, 0, NULL, &gic_options);
    if (ret)
    {
        set_basicauth_error(context, ret);
        goto end;
    }

end:
    krb5_free_cred_contents(context, &creds);

    return ret;
}