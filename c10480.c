read_yin_container(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin, int options,
                   struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lyxml_elem *sub, *next, root;
    struct lys_node *node = NULL;
    struct lys_node *retval;
    struct lys_node_container *cont;
    const char *value;
    void *reallocated;
    int r;
    int c_tpdf = 0, c_must = 0, c_ftrs = 0, c_ext = 0;

    /* init */
    memset(&root, 0, sizeof root);

    cont = calloc(1, sizeof *cont);
    LY_CHECK_ERR_RETURN(!cont, LOGMEM(ctx), NULL);

    cont->nodetype = LYS_CONTAINER;
    cont->prev = (struct lys_node *)cont;
    retval = (struct lys_node *)cont;

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

    /* process container's specific children */
    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions", "container", error);
            c_ext++;
        } else if (!strcmp(sub->name, "presence")) {
            if (cont->presence) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            GETVAL(ctx, value, sub, "value");
            cont->presence = lydict_insert(ctx, value, strlen(value));

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_PRESENCE, 0, unres)) {
                goto error;
            }
            lyxml_free(ctx, sub);
        } else if (!strcmp(sub->name, "when")) {
            if (cont->when) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }

            cont->when = read_yin_when(module, sub, unres);
            if (!cont->when) {
                lyxml_free(ctx, sub);
                goto error;
            }
            lyxml_free(ctx, sub);

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
        } else if (!strcmp(sub->name, "typedef")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_tpdf, cont->tpdf_size, "typedefs", "container", error);
            c_tpdf++;
        } else if (!strcmp(sub->name, "must")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_must, cont->must_size, "musts", "container", error);
            c_must++;
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "if-features", "container", error);
            c_ftrs++;
        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }
    }

    /* middle part - process nodes with cardinality of 0..n except the data nodes */
    if (c_tpdf) {
        cont->tpdf = calloc(c_tpdf, sizeof *cont->tpdf);
        LY_CHECK_ERR_GOTO(!cont->tpdf, LOGMEM(ctx), error);
    }
    if (c_must) {
        cont->must = calloc(c_must, sizeof *cont->must);
        LY_CHECK_ERR_GOTO(!cont->must, LOGMEM(ctx), error);
    }
    if (c_ftrs) {
        cont->iffeature = calloc(c_ftrs, sizeof *cont->iffeature);
        LY_CHECK_ERR_GOTO(!cont->iffeature, LOGMEM(ctx), error);
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
            r = fill_yin_typedef(module, retval, sub, &cont->tpdf[cont->tpdf_size], unres);
            cont->tpdf_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "must")) {
            r = fill_yin_must(module, sub, &cont->must[cont->must_size], unres);
            cont->must_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "if-feature")) {
            r = fill_yin_iffeature(retval, 0, sub, &cont->iffeature[cont->iffeature_size], unres);
            cont->iffeature_size++;
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
        }
        if (!node) {
            goto error;
        }

        lyxml_free(ctx, sub);
    }

    /* check XPath dependencies */
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && (cont->when || cont->must)) {
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
    lys_node_free(retval, NULL, 0);
    while (root.child) {
        lyxml_free(ctx, root.child);
    }
    return NULL;
}