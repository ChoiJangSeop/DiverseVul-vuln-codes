hook_connect_gnutls_set_certificates (gnutls_session_t tls_session,
                                      const gnutls_datum_t *req_ca, int nreq,
                                      const gnutls_pk_algorithm_t *pk_algos,
                                      int pk_algos_len,
                                      gnutls_retr_st *answer)
{
    struct t_hook *ptr_hook;
    int rc;
    
    rc = -1;
    ptr_hook = weechat_hooks[HOOK_TYPE_CONNECT];
    while (ptr_hook)
    {
        /* looking for the right hook using to the gnutls session pointer */
        if (!ptr_hook->deleted
            && HOOK_CONNECT(ptr_hook, gnutls_sess)
            && (*(HOOK_CONNECT(ptr_hook, gnutls_sess)) == tls_session))
        {
            rc = (int) (HOOK_CONNECT(ptr_hook, gnutls_cb))
                (ptr_hook->callback_data, tls_session, req_ca, nreq,
                 pk_algos, pk_algos_len, answer);
            break;
        }
        ptr_hook = ptr_hook->next_hook;
    }
    
    return rc;
}