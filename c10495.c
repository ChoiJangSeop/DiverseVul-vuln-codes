read_yin_input_output(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin,
                      int options, struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lyxml_elem *sub, *next, root;
    struct lys_node *node = NULL;
    struct lys_node *retval = NULL;
    struct lys_node_inout *inout;
    int r;
    int c_tpdf = 0, c_must = 0, c_ext = 0;

    /* init */
    memset(&root, 0, sizeof root);

    inout = calloc(1, sizeof *inout);
    LY_CHECK_ERR_RETURN(!inout, LOGMEM(ctx), NULL);
    inout->prev = (struct lys_node *)inout;

    if (!strcmp(yin->name, "input")) {
        inout->nodetype = LYS_INPUT;
        inout->name = lydict_insert(ctx, "input", 0);
    } else if (!strcmp(yin->name, "output")) {
        inout->nodetype = LYS_OUTPUT;
        inout->name = lydict_insert(ctx, "output", 0);
    } else {
        LOGINT(ctx);
        free(inout);
        goto error;
    }

    retval = (struct lys_node *)inout;
    retval->module = module;

    LOGDBG(LY_LDGYIN, "parsing %s statement \"%s\"", yin->name, retval->name);

    /* insert the node into the schema tree */
    if (lys_node_addchild(parent, lys_main_module(module), retval, options)) {
        goto error;
    }

    /* data statements */
    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (!sub->ns) {
            /* garbage */
            lyxml_free(ctx, sub);
        } else if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions",
                                          inout->nodetype == LYS_INPUT ? "input" : "output", error);
            c_ext++;
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
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_tpdf, inout->tpdf_size, "typedefs",
                                          inout->nodetype == LYS_INPUT ? "input" : "output", error);
            c_tpdf++;

        } else if ((module->version >= 2) && !strcmp(sub->name, "must")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_must, inout->must_size, "musts",
                                          inout->nodetype == LYS_INPUT ? "input" : "output", error);
            c_must++;

        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }
    }

    if (!root.child) {
        LOGVAL(ctx, LYE_MISSCHILDSTMT, LY_VLOG_LYS, retval, "schema-node", strnodetype(retval->nodetype));
        goto error;
    }

    /* middle part - process nodes with cardinality of 0..n except the data nodes */
    if (c_tpdf) {
        inout->tpdf = calloc(c_tpdf, sizeof *inout->tpdf);
        LY_CHECK_ERR_GOTO(!inout->tpdf, LOGMEM(ctx), error);
    }
    if (c_must) {
        inout->must = calloc(c_must, sizeof *inout->must);
        LY_CHECK_ERR_GOTO(!inout->must, LOGMEM(ctx), error);
    }
    if (c_ext) {
        inout->ext = calloc(c_ext, sizeof *inout->ext);
        LY_CHECK_ERR_GOTO(!inout->ext, LOGMEM(ctx), error);
    }

    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            r = lyp_yin_fill_ext(retval, LYEXT_PAR_NODE, 0, 0, module, sub, &retval->ext, &retval->ext_size, unres);
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "must")) {
            r = fill_yin_must(module, sub, &inout->must[inout->must_size], unres);
            inout->must_size++;
            if (r) {
                goto error;
            }
        } else { /* typedef */
            r = fill_yin_typedef(module, retval, sub, &inout->tpdf[inout->tpdf_size], unres);
            inout->tpdf_size++;
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
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && inout->must) {
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