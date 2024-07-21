lyd_new_anydata(struct lyd_node *parent, const struct lys_module *module, const char *name,
                void *value, LYD_ANYDATA_VALUETYPE value_type)
{
    const struct lys_node *siblings, *snode;

    if ((!parent && !module) || !name) {
        LOGARG;
        return NULL;
    }

    siblings = lyd_new_find_schema(parent, module, 0);
    if (!siblings) {
        LOGARG;
        return NULL;
    }

    if (lys_getnext_data(module, lys_parent(siblings), name, strlen(name), LYS_ANYDATA, &snode) || !snode) {
        LOGERR(siblings->module->ctx, LY_EINVAL, "Failed to find \"%s\" as a sibling to \"%s:%s\".",
               name, lys_node_module(siblings)->name, siblings->name);
        return NULL;
    }

    return lyd_create_anydata(parent, snode, value, value_type);
}