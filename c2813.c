int authenticate_user_krb5pwd(const char *user, const char *pswd, const char *service, const char *default_realm)
{
    krb5_context    kcontext = NULL;
    krb5_error_code code;
    krb5_principal  client = NULL;
    krb5_principal  server = NULL;
    int             ret = 0;
    char            *name = NULL;
    char            *p = NULL;

    code = krb5_init_context(&kcontext);
    if (code)
    {
        PyErr_SetObject(BasicAuthException_class, Py_BuildValue("((s:i))",
                                                                "Cannot initialize Kerberos5 context", code));
        return 0;
    }

    ret = krb5_parse_name (kcontext, service, &server);

    if (ret)
    {
        set_basicauth_error(kcontext, ret);
        ret = 0;
        goto end;
    }

    code = krb5_unparse_name(kcontext, server, &name);
    if (code)
    {
        set_basicauth_error(kcontext, code);
        ret = 0;
        goto end;
    }
#ifdef PRINTFS
    printf("Using %s as server principal for password verification\n", name);
#endif
    free(name);
    name = NULL;

    name = (char *)malloc(256);
    p = strchr(user, '@');
    if (p == NULL)
    {
        snprintf(name, 256, "%s@%s", user, default_realm);
    }
    else
    {
        snprintf(name, 256, "%s", user);
    }

    code = krb5_parse_name(kcontext, name, &client);
    if (code)
    {
        set_basicauth_error(kcontext, code);
        ret = 0;
        goto end;
    }

    code = verify_krb5_user(kcontext, client, pswd, server);

    if (code)
    {
        ret = 0;
        goto end;
    }

    ret = 1;

end:
#ifdef PRINTFS
    printf("kerb_authenticate_user_krb5pwd ret=%d user=%s authtype=%s\n", ret, user, "Basic");
#endif
    if (name)
        free(name);
    if (client)
        krb5_free_principal(kcontext, client);
    if (server)
        krb5_free_principal(kcontext, server);
    krb5_free_context(kcontext);

    return ret;
}