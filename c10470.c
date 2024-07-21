read_yin_list(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin, int options,
              struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lys_node *retval, *node;
    struct lys_node_list *list;
    struct lyxml_elem *sub, *next, root, uniq;
    int r;
    int c_tpdf = 0, c_must = 0, c_uniq = 0, c_ftrs = 0, c_ext = 0;
    int f_ordr = 0, f_max = 0, f_min = 0;
    const char *value;
    char *auxs;
    unsigned long val;
    void *reallocated;

    /* init */
    memset(&root, 0, sizeof root);
    memset(&uniq, 0, sizeof uniq);

    list = calloc(1, sizeof *list);
    LY_CHECK_ERR_RETURN(!list, LOGMEM(ctx), NULL);

    list->nodetype = LYS_LIST;
    list->prev = (struct lys_node *)list;
    retval = (struct lys_node *)list;

    if (read_yin_common(module, parent, retval, LYEXT_PAR_NODE, yin,
            OPT_IDENT | OPT_MODULE | ((options & LYS_PARSE_OPT_CFG_IGNORE) ? OPT_CFG_IGNORE :
                (options & LYS_PARSE_OPT_CFG_NOINHERIT) ? OPT_CFG_PARSE : OPT_CFG_PARSE | OPT_CFG_INHERIT),
            unres)) {
        goto error;
    }

    LOGDBG(LY_LDGYIN, "parsing %s statement \"%s\"", yin->name, retval->name);

    /* insert the node into the schema tree */
    if (lys_node_addchild(parent, lys_main_module(module), retval, options)) {
        goto error;
    }

    /* process list's specific children */
    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions", "list", error);
            c_ext++;
            continue;

        /* data statements */
        } else if (!strcmp(sub->name, "container") ||
                !strcmp(sub->name, "leaf-list") ||
                !strcmp(sub->name, "leaf") ||
                !strcmp(sub->name, "list") ||
                !strcmp(sub->name, "choice") ||
                !strcmp(sub->name, "uses") ||
                !strcmp(sub->name, "grouping") ||
                !strcmp(sub->name, "anyxml") ||
                !strcmp(sub->name, "anydata") ||
                !strcmp(sub->name, "action") ||
                !strcmp(sub->name, "notification")) {
            lyxml_unlink_elem(ctx, sub, 2);
            lyxml_add_child(ctx, &root, sub);

            /* array counters */
        } else if (!strcmp(sub->name, "key")) {
            /* check cardinality 0..1 */
            if (list->keys_size) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, list->name);
                goto error;
            }

            /* count the number of keys */
            GETVAL(ctx, value, sub, "value");
            list->keys_str = lydict_insert(ctx, value, 0);
            while ((value = strpbrk(value, " \t\n"))) {
                list->keys_size++;
                while (isspace(*value)) {
                    value++;
                }
            }
            list->keys_size++;
            list->keys = calloc(list->keys_size, sizeof *list->keys);
            LY_CHECK_ERR_GOTO(!list->keys, LOGMEM(ctx), error);

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_KEY, 0, unres)) {
                goto error;
            }
            lyxml_free(ctx, sub);
        } else if (!strcmp(sub->name, "unique")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_uniq, list->unique_size, "uniques", "list", error);
            c_uniq++;
            lyxml_unlink_elem(ctx, sub, 2);
            lyxml_add_child(ctx, &uniq, sub);
        } else if (!strcmp(sub->name, "typedef")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_tpdf, list->tpdf_size, "typedefs", "list", error);
            c_tpdf++;
        } else if (!strcmp(sub->name, "must")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_must, list->must_size, "musts", "list", error);
            c_must++;
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "if-features", "list", error);
            c_ftrs++;

            /* optional stetments */
        } else if (!strcmp(sub->name, "ordered-by")) {
            if (f_ordr) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            /* just checking the flags in llist is not sufficient, we would
             * allow multiple ordered-by statements with the "system" value
             */
            f_ordr = 1;

            if (list->flags & LYS_CONFIG_R) {
                /* RFC 6020, 7.7.5 - ignore ordering when the list represents
                 * state data
                 */
                lyxml_free(ctx, sub);
                continue;
            }

            GETVAL(ctx, value, sub, "value");
            if (!strcmp(value, "user")) {
                list->flags |= LYS_USERORDERED;
            } else if (strcmp(value, "system")) {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                goto error;
            } /* else system is the default value, so we can ignore it */

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_ORDEREDBY, 0, unres)) {
                goto error;
            }
            lyxml_free(ctx, sub);
        } else if (!strcmp(sub->name, "min-elements")) {
            if (f_min) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            f_min = 1;

            GETVAL(ctx, value, sub, "value");
            while (isspace(value[0])) {
                value++;
            }

            /* convert it to uint32_t */
            errno = 0;
            auxs = NULL;
            val = strtoul(value, &auxs, 10);
            if (*auxs || value[0] == '-' || errno || val > UINT32_MAX) {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                goto error;
            }
            list->min = (uint32_t) val;
            if (list->max && (list->min > list->max)) {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "\"min-elements\" is bigger than \"max-elements\".");
                lyxml_free(ctx, sub);
                goto error;
            }
            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_MIN, 0, unres)) {
                goto error;
            }
            lyxml_free(ctx, sub);
        } else if (!strcmp(sub->name, "max-elements")) {
            if (f_max) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            f_max = 1;

            GETVAL(ctx, value, sub, "value");
            while (isspace(value[0])) {
                value++;
            }

            if (!strcmp(value, "unbounded")) {
                list->max = 0;;
            } else {
                /* convert it to uint32_t */
                errno = 0;
                auxs = NULL;
                val = strtoul(value, &auxs, 10);
                if (*auxs || value[0] == '-' || errno || val == 0 || val > UINT32_MAX) {
                    LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                    goto error;
                }
                list->max = (uint32_t) val;
                if (list->min > list->max) {
                    LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                    LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "\"max-elements\" is smaller than \"min-elements\".");
                    goto error;
                }
            }
            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_MAX, 0, unres)) {
                goto error;
            }
            lyxml_free(ctx, sub);
        } else if (!strcmp(sub->name, "when")) {
            if (list->when) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }

            list->when = read_yin_when(module, sub, unres);
            if (!list->when) {
                lyxml_free(ctx, sub);
                goto error;
            }
            lyxml_free(ctx, sub);
        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }
    }

    /* check - if list is configuration, key statement is mandatory
     * (but only if we are not in a grouping or augment, then the check is deferred) */
    for (node = retval; node && !(node->nodetype & (LYS_GROUPING | LYS_AUGMENT | LYS_EXT)); node = node->parent);
    if (!node && (list->flags & LYS_CONFIG_W) && !list->keys_str) {
        LOGVAL(ctx, LYE_MISSCHILDSTMT, LY_VLOG_LYS, retval, "key", "list");
        goto error;
    }

    /* middle part - process nodes with cardinality of 0..n except the data nodes */
    if (c_tpdf) {
        list->tpdf = calloc(c_tpdf, sizeof *list->tpdf);
        LY_CHECK_ERR_GOTO(!list->tpdf, LOGMEM(ctx), error);
    }
    if (c_must) {
        list->must = calloc(c_must, sizeof *list->must);
        LY_CHECK_ERR_GOTO(!list->must, LOGMEM(ctx), error);
    }
    if (c_ftrs) {
        list->iffeature = calloc(c_ftrs, sizeof *list->iffeature);
        LY_CHECK_ERR_GOTO(!list->iffeature, LOGMEM(ctx), error);
    }
    if (c_ext) {
        /* some extensions may be already present from the substatements */
        reallocated = realloc(retval->ext, (c_ext + retval->ext_size) * sizeof *retval->ext);
        LY_CHECK_ERR_GOTO(!reallocated, LOGMEM(ctx), error);
        retval->ext = reallocated;

        /* init memory */
        memset(&retval->ext[retval->ext_size], 0, c_ext * sizeof *retval->ext);
    }

    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            r = lyp_yin_fill_ext(retval, LYEXT_PAR_NODE, 0, 0, module, sub, &retval->ext, &retval->ext_size, unres);
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "typedef")) {
            r = fill_yin_typedef(module, retval, sub, &list->tpdf[list->tpdf_size], unres);
            list->tpdf_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "if-feature")) {
            r = fill_yin_iffeature(retval, 0, sub, &list->iffeature[list->iffeature_size], unres);
            list->iffeature_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "must")) {
            r = fill_yin_must(module, sub, &list->must[list->must_size], unres);
            list->must_size++;
            if (r) {
                goto error;
            }
        }
    }

    lyp_reduce_ext_list(&retval->ext, retval->ext_size, c_ext + retval->ext_size);

    /* last part - process data nodes */
    LY_TREE_FOR_SAFE(root.child, next, sub) {
        if (!strcmp(sub->name, "container")) {
            node = read_yin_container(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "leaf-list")) {
            node = read_yin_leaflist(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "leaf")) {
            node = read_yin_leaf(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "list")) {
            node = read_yin_list(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "choice")) {
            node = read_yin_choice(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "uses")) {
            node = read_yin_uses(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "grouping")) {
            node = read_yin_grouping(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "anyxml")) {
            node = read_yin_anydata(module, retval, sub, LYS_ANYXML, options, unres);
        } else if (!strcmp(sub->name, "anydata")) {
            node = read_yin_anydata(module, retval, sub, LYS_ANYDATA, options, unres);
        } else if (!strcmp(sub->name, "action")) {
            node = read_yin_rpc_action(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "notification")) {
            node = read_yin_notif(module, retval, sub, options, unres);
        } else {
            LOGINT(ctx);
            goto error;
        }
        if (!node) {
            goto error;
        }

        lyxml_free(ctx, sub);
    }

    if (list->keys_str) {
        if (unres_schema_add_node(module, unres, list, UNRES_LIST_KEYS, NULL) == -1) {
            goto error;
        }
    } /* else config false list without a key, key_str presence in case of config true is checked earlier */

    /* process unique statements */
    if (c_uniq) {
        list->unique = calloc(c_uniq, sizeof *list->unique);
        LY_CHECK_ERR_GOTO(!list->unique, LOGMEM(ctx), error);

        LY_TREE_FOR_SAFE(uniq.child, next, sub) {
            r = fill_yin_unique(module, retval, sub, &list->unique[list->unique_size], unres);
            list->unique_size++;
            if (r) {
                goto error;
            }

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub,
                                     LYEXT_SUBSTMT_UNIQUE, list->unique_size - 1, unres)) {
                goto error;
            }
            lyxml_free(ctx, sub);
        }
    }

    /* check XPath dependencies */
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && (list->when || list->must)) {
        if (options & LYS_PARSE_OPT_INGRP) {
            if (lyxp_node_check_syntax(retval)) {
                goto error;
            }
        } else {
            if (unres_schema_add_node(module, unres, retval, UNRES_XPATH, NULL) == -1) {
                goto error;
            }
        }
    }

    for (r = 0; r < retval->ext_size; ++r) {
        /* set flag, which represent LYEXT_OPT_VALID */
        if (retval->ext[r]->flags & LYEXT_OPT_VALID) {
            retval->flags |= LYS_VALID_EXT;
            if (retval->ext[r]->flags & LYEXT_OPT_VALID_SUBTREE) {
                retval->flags |= LYS_VALID_EXT_SUBTREE;
                break;
            }
        }
    }

    return retval;

error:

    lys_node_free(ctx, retval, NULL, 0);
    while (root.child) {
        lyxml_free(ctx, root.child);
    }
    while (uniq.child) {
        lyxml_free(ctx, uniq.child);
    }

    return NULL;
}