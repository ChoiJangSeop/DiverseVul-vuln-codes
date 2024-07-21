am_cache_entry_t *am_cache_new(server_rec *s, const char *key)
{
    am_cache_entry_t *t;
    am_mod_cfg_rec *mod_cfg;
    void *table;
    apr_time_t current_time;
    int i;
    apr_time_t age;
    int rv;
    char buffer[512];

    /* Check if we have a valid session key. We abort if we don't. */
    if(key == NULL || strlen(key) != AM_ID_LENGTH) {
        return NULL;
    }


    mod_cfg = am_get_mod_cfg(s);


    /* Lock the table. */
    if((rv = apr_global_mutex_lock(mod_cfg->lock)) != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s,
                     "apr_global_mutex_lock() failed [%d]: %s",
                     rv, apr_strerror(rv, buffer, sizeof(buffer)));
        return NULL;
    }

    table = apr_shm_baseaddr_get(mod_cfg->cache);

    /* Get current time. If we find a entry with expires <= the current
     * time, then we can use it.
     */
    current_time = apr_time_now();

    /* We will use 't' to remember the best/oldest entry. We
     * initalize it to the first entry in the table to simplify the
     * following code (saves test for t == NULL).
     */
    t = am_cache_entry_ptr(mod_cfg, table, 0);

    /* Iterate over the session table. Update 't' to match the "best"
     * entry (the least recently used). 't' will point a free entry
     * if we find one. Otherwise, 't' will point to the least recently
     * used entry.
     */
    for(i = 0; i < mod_cfg->init_cache_size; i++) {
        am_cache_entry_t *e = am_cache_entry_ptr(mod_cfg, table, i);
        if (e->key[0] == '\0') {
            /* This entry is free. Update 't' to this entry
             * and exit loop.
             */
            t = e;
            break;
        }

        if (e->expires <= current_time) {
            /* This entry is expired, and is therefore free.
             * Update 't' and exit loop.
             */
            t = e;
            break;
        }

        if (e->access < t->access) {
            /* This entry is older than 't' - update 't'. */
            t = e;
        }
    }


    if(t->key[0] != '\0' && t->expires > current_time) {
        /* We dropped a LRU entry. Calculate the age in seconds. */
        age = (current_time - t->access) / 1000000;

        if(age < 3600) {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s,
                         "Dropping LRU entry entry with age = %" APR_TIME_T_FMT
                         "s, which is less than one hour. It may be a good"
                         " idea to increase MellonCacheSize.",
                         age);
        }
    }

    /* Now 't' points to the entry we are going to use. We initialize
     * it and returns it.
     */

    strcpy(t->key, key);

    /* Far far into the future. */
    t->expires = 0x7fffffffffffffffLL;

    t->logged_in = 0;
    t->size = 0;

    am_cache_storage_null(&t->user);
    am_cache_storage_null(&t->lasso_identity);
    am_cache_storage_null(&t->lasso_session);
    am_cache_storage_null(&t->lasso_saml_response);
    am_cache_entry_env_null(t);

    t->pool_size = am_cache_entry_pool_size(mod_cfg);
    t->pool[0] = '\0';
    t->pool_used = 1;

    return t;
}