static apr_status_t session_identity_decode(request_rec * r, session_rec * z)
{

    char *last = NULL;
    char *encoded, *pair;
    const char *sep = "&";

    /* sanity check - anything to decode? */
    if (!z->encoded) {
        return OK;
    }

    /* decode what we have */
    encoded = apr_pstrdup(r->pool, z->encoded);
    pair = apr_strtok(encoded, sep, &last);
    while (pair && pair[0]) {
        char *plast = NULL;
        const char *psep = "=";
        char *key = apr_strtok(pair, psep, &plast);
        char *val = apr_strtok(NULL, psep, &plast);
        if (key && *key) {
            if (!val || !*val) {
                apr_table_unset(z->entries, key);
            }
            else if (!ap_unescape_urlencoded(key) && !ap_unescape_urlencoded(val)) {
                if (!strcmp(SESSION_EXPIRY, key)) {
                    z->expiry = (apr_time_t) apr_atoi64(val);
                }
                else {
                    apr_table_set(z->entries, key, val);
                }
            }
        }
        pair = apr_strtok(NULL, sep, &last);
    }
    z->encoded = NULL;
    return OK;

}