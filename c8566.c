lyd_new_output(struct lyd_node *parent, const struct lys_module *module, const char *name)
{
    const struct lys_node *snode = NULL, *siblings;

    if ((!parent && !module) || !name) {
        LOGARG;
        return NULL;
    }

    siblings = lyd_new_find_schema(parent, module, 1);
    if (!siblings) {
        LOGARG;
        return NULL;
    }

    if (lys_getnext_data(module, lys_parent(siblings), name, strlen(name), LYS_CONTAINER | LYS_LIST | LYS_NOTIF
                         | LYS_RPC | LYS_ACTION, &snode) || !snode) {
        LOGERR(siblings->module->ctx, LY_EINVAL, "Failed to find \"%s\" as a sibling to \"%s:%s\".",
               name, lys_node_module(siblings)->name, siblings->name);
        return NULL;
    }

    return _lyd_new(parent, snode, 0);
}