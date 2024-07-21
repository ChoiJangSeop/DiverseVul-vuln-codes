read_yin_case(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin, int options,
              struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lyxml_elem *sub, *next, root;
    struct lys_node_case *cs;
    struct lys_node *retval, *node = NULL;
    int c_ftrs = 0, c_ext = 0, ret;
    void *reallocated;

    /* init */
    memset(&root, 0, sizeof root);

    cs = calloc(1, sizeof *cs);
    LY_CHECK_ERR_RETURN(!cs, LOGMEM(ctx), NULL);
    cs->nodetype = LYS_CASE;
    cs->prev = (struct lys_node *)cs;
    retval = (struct lys_node *)cs;

    if (read_yin_common(module, parent, retval, LYEXT_PAR_NODE, yin,
            OPT_IDENT | OPT_MODULE | (!(options & LYS_PARSE_OPT_CFG_MASK) ? OPT_CFG_INHERIT : 0), unres)) {
        goto error;
    }

    LOGDBG(LY_LDGYIN, "parsing %s statement \"%s\"", yin->name, retval->name);

    /* insert the node into the schema tree */
    if (lys_node_addchild(parent, lys_main_module(module), retval, options)) {
        goto error;
    }

    /* process choice's specific children */
    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions", "case", error);
            c_ext++;
        } else if (!strcmp(sub->name, "container") ||
                !strcmp(sub->name, "leaf-list") ||
                !strcmp(sub->name, "leaf") ||
                !strcmp(sub->name, "list") ||
                !strcmp(sub->name, "uses") ||
                !strcmp(sub->name, "choice") ||
                !strcmp(sub->name, "anyxml") ||
                !strcmp(sub->name, "anydata")) {

            lyxml_unlink_elem(ctx, sub, 2);
            lyxml_add_child(ctx, &root, sub);
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "if-features", "case", error);
            c_ftrs++;
        } else if (!strcmp(sub->name, "when")) {
            if (cs->when) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }

            cs->when = read_yin_when(module, sub, unres);
            if (!cs->when) {
                goto error;
            }

            lyxml_free(ctx, sub);
        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }
    }

    if (c_ftrs) {
        cs->iffeature = calloc(c_ftrs, sizeof *cs->iffeature);
        LY_CHECK_ERR_GOTO(!cs->iffeature, LOGMEM(ctx), error);
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
            ret = lyp_yin_fill_ext(retval, LYEXT_PAR_NODE, 0, 0, module, sub, &retval->ext, &retval->ext_size, unres);
            if (ret) {
                goto error;
            }
        } else {
            /* if-feature */
            ret = fill_yin_iffeature(retval, 0, sub, &cs->iffeature[cs->iffeature_size], unres);
            cs->iffeature_size++;
            if (ret) {
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
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && cs->when) {
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
    while (root.child) {
        lyxml_free(ctx, root.child);
    }
    lys_node_free(retval, NULL, 0);

    return NULL;
}