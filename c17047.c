static int tls1_check_sig_alg(SSL *s, X509 *x, int default_nid)
{
    int sig_nid;
    size_t i;
    if (default_nid == -1)
        return 1;
    sig_nid = X509_get_signature_nid(x);
    if (default_nid)
        return sig_nid == default_nid ? 1 : 0;
    for (i = 0; i < s->shared_sigalgslen; i++)
        if (sig_nid == s->shared_sigalgs[i]->sigandhash)
            return 1;
    return 0;
}