read_yin_notif(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin,
               int options, struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lyxml_elem *sub, *next, root;
    struct lys_node *node = NULL;
    struct lys_node *retval;
    struct lys_node_notif *notif;
    int r;
    int c_tpdf = 0, c_ftrs = 0, c_must = 0, c_ext = 0;
    void *reallocated;

    if (parent && (module->version < 2)) {
        LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, parent, "notification");
        return NULL;
    }

    memset(&root, 0, sizeof root);

    notif = calloc(1, sizeof *notif);
    LY_CHECK_ERR_RETURN(!notif, LOGMEM(ctx), NULL);

    notif->nodetype = LYS_NOTIF;
    notif->prev = (struct lys_node *)notif;
    retval = (struct lys_node *)notif;

    if (read_yin_common(module, parent, retval, LYEXT_PAR_NODE, yin, OPT_IDENT | OPT_MODULE, unres)) {
        goto error;
    }

    LOGDBG(LY_LDGYIN, "parsing %s statement \"%s\"", yin->name, retval->name);

    /* insert the node into the schema tree */
    if (lys_node_addchild(parent, lys_main_module(module), retval, options)) {
        goto error;
    }

    /* process rpc's specific children */
    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions", "notification", error);
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
                !strcmp(sub->name, "anydata")) {
            lyxml_unlink_elem(ctx, sub, 2);
            lyxml_add_child(ctx, &root, sub);

            /* array counters */
        } else if (!strcmp(sub->name, "typedef")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_tpdf, notif->tpdf_size, "typedefs", "notification", error);
            c_tpdf++;
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "if-features", "notification", error);
            c_ftrs++;
        } else if ((module->version >= 2) && !strcmp(sub->name, "must")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_must, notif->must_size, "musts", "notification", error);
            c_must++;
        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }
    }

    /* middle part - process nodes with cardinality of 0..n except the data nodes */
    if (c_tpdf) {
        notif->tpdf = calloc(c_tpdf, sizeof *notif->tpdf);
        LY_CHECK_ERR_GOTO(!notif->tpdf, LOGMEM(ctx), error);
    }
    if (c_ftrs) {
        notif->iffeature = calloc(c_ftrs, sizeof *notif->iffeature);
        LY_CHECK_ERR_GOTO(!notif->iffeature, LOGMEM(ctx), error);
    }
    if (c_must) {
        notif->must = calloc(c_must, sizeof *notif->must);
        LY_CHECK_ERR_GOTO(!notif->must, LOGMEM(ctx), error);
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
            r = fill_yin_typedef(module, retval, sub, &notif->tpdf[notif->tpdf_size], unres);
            notif->tpdf_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "if-feature")) {
            r = fill_yin_iffeature(retval, 0, sub, &notif->iffeature[notif->iffeature_size], unres);
            notif->iffeature_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "must")) {
            r = fill_yin_must(module, sub, &notif->must[notif->must_size], unres);
            notif->must_size++;
            if (r) {
                goto error;
            }
        }
    }

    lyp_reduce_ext_list(&retval->ext, retval->ext_size, c_ext + retval->ext_size);

    /* last part - process data nodes */
    options |= LYS_PARSE_OPT_CFG_IGNORE;
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
        }
        if (!node) {
            goto error;
        }

        lyxml_free(ctx, sub);
    }

    /* check XPath dependencies */
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && notif->must) {
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

    return retval;

error:
    lys_node_free(retval, NULL, 0);
    while (root.child) {
        lyxml_free(ctx, root.child);
    }
    return NULL;
}