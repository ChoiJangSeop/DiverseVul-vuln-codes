ldbm_config_search_entry_callback(Slapi_PBlock *pb __attribute__((unused)),
                                  Slapi_Entry *e,
                                  Slapi_Entry *entryAfter __attribute__((unused)),
                                  int *returncode,
                                  char *returntext,
                                  void *arg)
{
    char buf[BUFSIZ];
    struct berval *vals[2];
    struct berval val;
    struct ldbminfo *li = (struct ldbminfo *)arg;
    config_info *config;
    int scope;

    vals[0] = &val;
    vals[1] = NULL;

    returntext[0] = '\0';

    PR_Lock(li->li_config_mutex);

    if (pb) {
        slapi_pblock_get(pb, SLAPI_SEARCH_SCOPE, &scope);
        if (scope == LDAP_SCOPE_BASE) {
            char **attrs = NULL;
            slapi_pblock_get(pb, SLAPI_SEARCH_ATTRS, &attrs);
            if (attrs) {
                for (size_t i = 0; attrs[i]; i++) {
                    if (ldbm_config_moved_attr(attrs[i])) {
                        slapi_pblock_set(pb, SLAPI_PB_RESULT_TEXT, "at least one required attribute has been moved to the BDB scecific configuration entry");
                        break;
                    }
                }
            }
        
        }
    }

    for (config = ldbm_config; config->config_name != NULL; config++) {
        /* Go through the ldbm_config table and fill in the entry. */

        if (!(config->config_flags & (CONFIG_FLAG_ALWAYS_SHOW | CONFIG_FLAG_PREVIOUSLY_SET))) {
            /* This config option shouldn't be shown */
            continue;
        }

        ldbm_config_get((void *)li, config, buf);

        val.bv_val = buf;
        val.bv_len = strlen(buf);
        slapi_entry_attr_replace(e, config->config_name, vals);
    }

    PR_Unlock(li->li_config_mutex);

    *returncode = LDAP_SUCCESS;
    return SLAPI_DSE_CALLBACK_OK;
}