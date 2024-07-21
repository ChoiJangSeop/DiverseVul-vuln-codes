static int ipa_extdom_init_ctx(Slapi_PBlock *pb, struct ipa_extdom_ctx **_ctx)
{
    struct ipa_extdom_ctx *ctx;
    Slapi_Entry *e;
    int ret;

    ctx = calloc(1, sizeof(struct ipa_extdom_ctx));
    if (!ctx) {
        return LDAP_OPERATIONS_ERROR;
    }

    ret = slapi_pblock_get(pb, SLAPI_PLUGIN_IDENTITY, &ctx->plugin_id);
    if ((ret != 0) || (NULL == ctx->plugin_id)) {
        LOG_FATAL("Could not get identity or identity was NULL\n");
        if (ret == 0) {
            ret = -1;
        }
        goto done;
    }

    slapi_pblock_get(pb, SLAPI_PLUGIN_CONFIG_ENTRY, &e);
    if (!e) {
        LOG_FATAL("Plugin configuration not found!\n");
        ret = -1;
        goto done;
    }

    ctx->base_dn = slapi_entry_attr_get_charptr(e, "nsslapd-basedn");
    if (!ctx->base_dn) {
        LOG_FATAL("Base DN not found in plugin configuration not found!\n");
        ret = -1;
        goto done;
    }


done:
    if (ret) {
        free(ctx);
    } else {
        *_ctx = ctx;
    }
    return ret;
}