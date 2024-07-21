resolve_schema_leafref_predicate(const char *path, const struct lys_node *context_node, struct lys_node *parent)
{
    const struct lys_module *trg_mod;
    const struct lys_node *src_node, *dst_node;
    const char *path_key_expr, *source, *sour_pref, *dest, *dest_pref;
    int pke_len, sour_len, sour_pref_len, dest_len, dest_pref_len, pke_parsed, parsed = 0;
    int has_predicate, dest_parent_times, i, rc;
    struct ly_ctx *ctx = context_node->module->ctx;

    do {
        if ((i = parse_path_predicate(path, &sour_pref, &sour_pref_len, &source, &sour_len, &path_key_expr,
                                      &pke_len, &has_predicate)) < 1) {
            LOGVAL(ctx, LYE_INCHAR, LY_VLOG_LYS, parent, path[-i], path-i);
            return -parsed+i;
        }
        parsed += i;
        path += i;

        /* source (must be leaf) */
        if (sour_pref) {
            trg_mod = lyp_get_module(lys_node_module(parent), NULL, 0, sour_pref, sour_pref_len, 0);
        } else {
            trg_mod = lys_node_module(parent);
        }
        rc = lys_getnext_data(trg_mod, context_node, source, sour_len, LYS_LEAF | LYS_LEAFLIST, &src_node);
        if (rc) {
            LOGVAL(ctx, LYE_NORESOLV, LY_VLOG_LYS, parent, "leafref predicate", path-parsed);
            return 0;
        }

        /* destination */
        dest_parent_times = 0;
        pke_parsed = 0;
        if ((i = parse_path_key_expr(path_key_expr, &dest_pref, &dest_pref_len, &dest, &dest_len,
                                     &dest_parent_times)) < 1) {
            LOGVAL(ctx, LYE_INCHAR, LY_VLOG_LYS, parent, path_key_expr[-i], path_key_expr-i);
            return -parsed;
        }
        pke_parsed += i;

        for (i = 0, dst_node = parent; i < dest_parent_times; ++i) {
            if (!dst_node) {
                /* we went too much into parents, there is no parent anymore */
                LOGVAL(ctx, LYE_NORESOLV, LY_VLOG_LYS, parent, "leafref predicate", path_key_expr);
                return 0;
            }

            if (dst_node->parent && (dst_node->parent->nodetype == LYS_AUGMENT)
                    && !((struct lys_node_augment *)dst_node->parent)->target) {
                /* we are in an unresolved augment, cannot evaluate */
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_LYS, dst_node->parent,
                       "Cannot resolve leafref predicate \"%s\" because it is in an unresolved augment.", path_key_expr);
                return 0;
            }

            /* path is supposed to be evaluated in data tree, so we have to skip
             * all schema nodes that cannot be instantiated in data tree */
            for (dst_node = lys_parent(dst_node);
                 dst_node && !(dst_node->nodetype & (LYS_CONTAINER | LYS_LIST | LYS_ACTION | LYS_NOTIF | LYS_RPC));
                 dst_node = lys_parent(dst_node));
        }
        while (1) {
            if (dest_pref) {
                trg_mod = lyp_get_module(lys_node_module(parent), NULL, 0, dest_pref, dest_pref_len, 0);
            } else {
                trg_mod = lys_node_module(parent);
            }
            rc = lys_getnext_data(trg_mod, dst_node, dest, dest_len, LYS_CONTAINER | LYS_LIST | LYS_LEAF, &dst_node);
            if (rc) {
                LOGVAL(ctx, LYE_NORESOLV, LY_VLOG_LYS, parent, "leafref predicate", path_key_expr);
                return 0;
            }

            if (pke_len == pke_parsed) {
                break;
            }

            if ((i = parse_path_key_expr(path_key_expr + pke_parsed, &dest_pref, &dest_pref_len, &dest, &dest_len,
                                         &dest_parent_times)) < 1) {
                LOGVAL(ctx, LYE_INCHAR, LY_VLOG_LYS, parent,
                       (path_key_expr + pke_parsed)[-i], (path_key_expr + pke_parsed)-i);
                return -parsed;
            }
            pke_parsed += i;
        }

        /* check source - dest match */
        if (dst_node->nodetype != src_node->nodetype) {
            LOGVAL(ctx, LYE_NORESOLV, LY_VLOG_LYS, parent, "leafref predicate", path - parsed);
            LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "Destination node is not a %s, but a %s.",
                   strnodetype(src_node->nodetype), strnodetype(dst_node->nodetype));
            return -parsed;
        }
    } while (has_predicate);

    return parsed;
}