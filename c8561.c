lyd_dup_to_ctx(const struct lyd_node *node, int options, struct ly_ctx *ctx)
{
    struct ly_ctx *log_ctx;
    struct lys_node_list *slist;
    struct lys_node *schema;
    const char *yang_data_name;
    const struct lys_module *trg_mod;
    const struct lyd_node *next, *elem;
    struct lyd_node *ret, *parent, *key, *key_dup, *new_node = NULL;
    uint16_t i;

    if (!node) {
        LOGARG;
        return NULL;
    }

    log_ctx = (ctx ? ctx : node->schema->module->ctx);
    if (ctx == node->schema->module->ctx) {
        /* target context is actually the same as the source context,
         * ignore the target context */
        ctx = NULL;
    }

    ret = NULL;
    parent = NULL;

    /* LY_TREE_DFS */
    for (elem = next = node; elem; elem = next) {

        /* find the correct schema */
        if (ctx) {
            schema = NULL;
            if (parent) {
                trg_mod = lyp_get_module(parent->schema->module, NULL, 0, lyd_node_module(elem)->name,
                                         strlen(lyd_node_module(elem)->name), 1);
                if (!trg_mod) {
                    LOGERR(log_ctx, LY_EINVAL, "Target context does not contain model for the data node being duplicated (%s).",
                                lyd_node_module(elem)->name);
                    goto error;
                }
                /* we know its parent, so we can start with it */
                lys_getnext_data(trg_mod, parent->schema, elem->schema->name, strlen(elem->schema->name),
                                 elem->schema->nodetype, (const struct lys_node **)&schema);
            } else {
                /* we have to search in complete context */
                schema = lyd_get_schema_inctx(elem, ctx);
            }

            if (!schema) {
                yang_data_name = lyp_get_yang_data_template_name(elem);
                if (yang_data_name) {
                    LOGERR(log_ctx, LY_EINVAL, "Target context does not contain schema node for the data node being duplicated "
                                        "(%s:#%s/%s).", lyd_node_module(elem)->name, yang_data_name, elem->schema->name);
                } else {
                    LOGERR(log_ctx, LY_EINVAL, "Target context does not contain schema node for the data node being duplicated "
                                        "(%s:%s).", lyd_node_module(elem)->name, elem->schema->name);
                }
                goto error;
            }
        } else {
            schema = elem->schema;
        }

        /* make node copy */
        new_node = _lyd_dup_node(elem, schema, log_ctx, options);
        if (!new_node) {
            goto error;
        }

        if (parent && lyd_insert(parent, new_node)) {
            goto error;
        }

        if (!ret) {
            ret = new_node;
        }

        if (!(options & LYD_DUP_OPT_RECURSIVE)) {
            break;
        }

        /* LY_TREE_DFS_END */
        /* select element for the next run - children first,
         * child exception for lyd_node_leaf and lyd_node_leaflist */
        if (elem->schema->nodetype & (LYS_LEAF | LYS_LEAFLIST | LYS_ANYDATA)) {
            next = NULL;
        } else {
            next = elem->child;
        }
        if (!next) {
            if (elem->parent == node->parent) {
                break;
            }
            /* no children, so try siblings */
            next = elem->next;
        } else {
            parent = new_node;
        }
        new_node = NULL;

        while (!next) {
            /* no siblings, go back through parents */
            elem = elem->parent;
            if (elem->parent == node->parent) {
                break;
            }
            if (!parent) {
                LOGINT(log_ctx);
                goto error;
            }
            parent = parent->parent;
            /* parent is already processed, go to its sibling */
            next = elem->next;
        }
    }

    /* dup all the parents */
    if (options & LYD_DUP_OPT_WITH_PARENTS) {
        parent = ret;
        for (elem = node->parent; elem; elem = elem->parent) {
            new_node = lyd_dup(elem, options & LYD_DUP_OPT_NO_ATTR);
            LY_CHECK_ERR_GOTO(!new_node, LOGMEM(log_ctx), error);

            /* dup all list keys */
            if (new_node->schema->nodetype == LYS_LIST) {
                slist = (struct lys_node_list *)new_node->schema;
                for (key = elem->child, i = 0; key && (i < slist->keys_size); ++i, key = key->next) {
                    if (key->schema != (struct lys_node *)slist->keys[i]) {
                        LOGVAL(log_ctx, LYE_PATH_INKEY, LY_VLOG_LYD, new_node, slist->keys[i]->name);
                        goto error;
                    }

                    key_dup = lyd_dup(key, options & LYD_DUP_OPT_NO_ATTR);
                    LY_CHECK_ERR_GOTO(!key_dup, LOGMEM(log_ctx), error);

                    if (lyd_insert(new_node, key_dup)) {
                        lyd_free(key_dup);
                        goto error;
                    }
                }
                if (!key && (i < slist->keys_size)) {
                    LOGVAL(log_ctx, LYE_PATH_INKEY, LY_VLOG_LYD, new_node, slist->keys[i]->name);
                    goto error;
                }
            }

            /* link together */
            if (lyd_insert(new_node, parent)) {
                ret = parent;
                goto error;
            }
            parent = new_node;
        }
    }

    return ret;

error:
    lyd_free(ret);
    return NULL;
}