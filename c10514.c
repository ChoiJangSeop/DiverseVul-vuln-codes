static apr_status_t session_identity_encode(request_rec * r, session_rec * z)
{

    char *buffer = NULL;
    int length = 0;
    if (z->expiry) {
        char *expiry = apr_psprintf(z->pool, "%" APR_INT64_T_FMT, z->expiry);
        apr_table_setn(z->entries, SESSION_EXPIRY, expiry);
    }
    apr_table_do(identity_count, &length, z->entries, NULL);
    buffer = apr_pcalloc(r->pool, length + 1);
    apr_table_do(identity_concat, buffer, z->entries, NULL);
    z->encoded = buffer;
    return OK;

}