Perl_my_setenv(pTHX_ const char *nam, const char *val)
{
    dVAR;
    char *envstr;
    const int nlen = strlen(nam);
    int vlen;

    if (!val) {
       val = "";
    }
    vlen = strlen(val);
    Newx(envstr, nlen+vlen+2, char);
    my_setenv_format(envstr, nam, nlen, val, vlen);
    (void)PerlEnv_putenv(envstr);
    Safefree(envstr);
}