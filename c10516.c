AP_DECLARE(int) ap_location_walk(request_rec *r)
{
    ap_conf_vector_t *now_merged = NULL;
    core_server_config *sconf =
        ap_get_core_module_config(r->server->module_config);
    ap_conf_vector_t **sec_ent = (ap_conf_vector_t **)sconf->sec_url->elts;
    int num_sec = sconf->sec_url->nelts;
    walk_cache_t *cache;
    const char *entry_uri;
    int cached;

    /* No tricks here, there are no <Locations > to parse in this vhost.
     * We won't destroy the cache, just in case _this_ redirect is later
     * redirected again to a vhost with <Location > blocks to optimize.
     */
    if (!num_sec) {
        return OK;
    }

    cache = prep_walk_cache(AP_NOTE_LOCATION_WALK, r);
    cached = (cache->cached != NULL);
    entry_uri = r->uri;

    /* If we have an cache->cached location that matches r->uri,
     * and the vhost's list of locations hasn't changed, we can skip
     * rewalking the location_walk entries.
     */
    if (cached
        && (cache->dir_conf_tested == sec_ent)
        && (strcmp(entry_uri, cache->cached) == 0)) {
        /* Well this looks really familiar!  If our end-result (per_dir_result)
         * didn't change, we have absolutely nothing to do :)
         * Otherwise (as is the case with most dir_merged/file_merged requests)
         * we must merge our dir_conf_merged onto this new r->per_dir_config.
         */
        if (r->per_dir_config == cache->per_dir_result) {
            return OK;
        }

        if (cache->walked->nelts) {
            now_merged = ((walk_walked_t*)cache->walked->elts)
                                            [cache->walked->nelts - 1].merged;
        }
    }
    else {
        /* We start now_merged from NULL since we want to build
         * a locations list that can be merged to any vhost.
         */
        int len, sec_idx;
        int matches = cache->walked->nelts;
        int cached_matches = matches;
        walk_walked_t *last_walk = (walk_walked_t*)cache->walked->elts;
        apr_pool_t *rxpool = NULL;

        cached &= auth_internal_per_conf;
        cache->cached = entry_uri;

        /* Go through the location entries, and check for matches.
         * We apply the directive sections in given order, we should
         * really try them with the most general first.
         */
        for (sec_idx = 0; sec_idx < num_sec; ++sec_idx) {

            core_dir_config *entry_core;
            entry_core = ap_get_core_module_config(sec_ent[sec_idx]);

            /* ### const strlen can be optimized in location config parsing */
            len = strlen(entry_core->d);

            /* Test the regex, fnmatch or string as appropriate.
             * If it's a strcmp, and the <Location > pattern was
             * not slash terminated, then this uri must be slash
             * terminated (or at the end of the string) to match.
             */
            if (entry_core->r) {

                int nmatch = 0;
                int i;
                ap_regmatch_t *pmatch = NULL;

                if (entry_core->refs && entry_core->refs->nelts) {
                    if (!rxpool) {
                        apr_pool_create(&rxpool, r->pool);
                        apr_pool_tag(rxpool, "location_walk_rxpool");
                    }
                    nmatch = entry_core->refs->nelts;
                    pmatch = apr_palloc(rxpool, nmatch*sizeof(ap_regmatch_t));
                }

                if (ap_regexec(entry_core->r, entry_uri, nmatch, pmatch, 0)) {
                    continue;
                }

                for (i = 0; i < nmatch; i++) {
                    if (pmatch[i].rm_so >= 0 && pmatch[i].rm_eo >= 0 && 
                        ((const char **)entry_core->refs->elts)[i]) {
                        apr_table_setn(r->subprocess_env,
                                       ((const char **)entry_core->refs->elts)[i],
                                       apr_pstrndup(r->pool,
                                       entry_uri + pmatch[i].rm_so,
                                       pmatch[i].rm_eo - pmatch[i].rm_so));
                    }
                }

            }
            else {

                if ((entry_core->d_is_fnmatch
                   ? apr_fnmatch(entry_core->d, cache->cached, APR_FNM_PATHNAME)
                   : (strncmp(entry_core->d, cache->cached, len)
                      || (len > 0
                          && entry_core->d[len - 1] != '/'
                          && cache->cached[len] != '/'
                          && cache->cached[len] != '\0')))) {
                    continue;
                }

            }

            /* If we merged this same section last time, reuse it
             */
            if (matches) {
                if (last_walk->matched == sec_ent[sec_idx]) {
                    now_merged = last_walk->merged;
                    ++last_walk;
                    --matches;
                    continue;
                }

                /* We fell out of sync.  This is our own copy of walked,
                 * so truncate the remaining matches and reset remaining.
                 */
                cache->walked->nelts -= matches;
                matches = 0;
                cached = 0;
            }

            if (now_merged) {
                now_merged = ap_merge_per_dir_configs(r->pool,
                                                      now_merged,
                                                      sec_ent[sec_idx]);
            }
            else {
                now_merged = sec_ent[sec_idx];
            }

            last_walk = (walk_walked_t*)apr_array_push(cache->walked);
            last_walk->matched = sec_ent[sec_idx];
            last_walk->merged = now_merged;
        }

        if (rxpool) {
            apr_pool_destroy(rxpool);
        }

        /* Whoops - everything matched in sequence, but either the original
         * walk found some additional matches (which we need to truncate), or
         * this walk found some additional matches.
         */
        if (matches) {
            cache->walked->nelts -= matches;
            cached = 0;
        }
        else if (cache->walked->nelts > cached_matches) {
            cached = 0;
        }
    }

    if (cached
        && r->per_dir_config == cache->dir_conf_merged) {
        r->per_dir_config = cache->per_dir_result;
        return OK;
    }

    cache->dir_conf_tested = sec_ent;
    cache->dir_conf_merged = r->per_dir_config;

    /* Merge our cache->dir_conf_merged construct with the r->per_dir_configs,
     * and note the end result to (potentially) skip this step next time.
     */
    if (now_merged) {
        r->per_dir_config = ap_merge_per_dir_configs(r->pool,
                                                     r->per_dir_config,
                                                     now_merged);
    }
    cache->per_dir_result = r->per_dir_config;

    return OK;
}