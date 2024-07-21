lyd_new_yangdata(const struct lys_module *module, const char *name_template, const char *name)
{
    const struct lys_node *schema = NULL, *snode;

    if (!module || !name_template || !name) {
        LOGARG;
        return NULL;
    }

    schema = lyp_get_yang_data_template(module, name_template, strlen(name_template));
    if (!schema) {
        LOGERR(module->ctx, LY_EINVAL, "Failed to find yang-data template \"%s\".", name_template);
        return NULL;
    }

    if (lys_getnext_data(module, schema, name, strlen(name), LYS_CONTAINER, &snode) || !snode) {
        LOGERR(module->ctx, LY_EINVAL, "Failed to find \"%s\" as a container child of \"%s:%s\".",
               name, module->name, schema->name);
        return NULL;
    }

    return _lyd_new(NULL, snode, 0);
}