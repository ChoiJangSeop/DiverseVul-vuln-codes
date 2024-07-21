read_yin_rpc_action(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin,
                    int options, struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lyxml_elem *sub, *next, root;
    struct lys_node *node = NULL;
    struct lys_node *retval;
    struct lys_node_rpc_action *rpc;
    int r;
    int c_tpdf = 0, c_ftrs = 0, c_input = 0, c_output = 0, c_ext = 0;
    void *reallocated;

    if (!strcmp(yin->name, "action") && (module->version < 2)) {
        LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, parent, "action");
        return NULL;
    }

    /* init */
    memset(&root, 0, sizeof root);

    rpc = calloc(1, sizeof *rpc);
    LY_CHECK_ERR_RETURN(!rpc, LOGMEM(ctx), NULL);

    rpc->nodetype = (!strcmp(yin->name, "rpc") ? LYS_RPC : LYS_ACTION);
    rpc->prev = (struct lys_node *)rpc;
    retval = (struct lys_node *)rpc;

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
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions",
                                          rpc->nodetype == LYS_RPC ? "rpc" : "action", error);
            c_ext++;
            continue;
        } else if (!strcmp(sub->name, "input")) {
            if (c_input) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            c_input++;
            lyxml_unlink_elem(ctx, sub, 2);
            lyxml_add_child(ctx, &root, sub);
        } else if (!strcmp(sub->name, "output")) {
            if (c_output) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            c_output++;
            lyxml_unlink_elem(ctx, sub, 2);
            lyxml_add_child(ctx, &root, sub);

            /* data statements */
        } else if (!strcmp(sub->name, "grouping")) {
            lyxml_unlink_elem(ctx, sub, 2);
            lyxml_add_child(ctx, &root, sub);

            /* array counters */
        } else if (!strcmp(sub->name, "typedef")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_tpdf, rpc->tpdf_size, "typedefs",
                                          rpc->nodetype == LYS_RPC ? "rpc" : "action", error);
            c_tpdf++;
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "if-features",
                                          rpc->nodetype == LYS_RPC ? "rpc" : "action", error);
            c_ftrs++;
        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }
    }

    /* middle part - process nodes with cardinality of 0..n except the data nodes */
    if (c_tpdf) {
        rpc->tpdf = calloc(c_tpdf, sizeof *rpc->tpdf);
        LY_CHECK_ERR_GOTO(!rpc->tpdf, LOGMEM(ctx), error);
    }
    if (c_ftrs) {
        rpc->iffeature = calloc(c_ftrs, sizeof *rpc->iffeature);
        LY_CHECK_ERR_GOTO(!rpc->iffeature, LOGMEM(ctx), error);
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
            r = fill_yin_typedef(module, retval, sub, &rpc->tpdf[rpc->tpdf_size], unres);
            rpc->tpdf_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "if-feature")) {
            r = fill_yin_iffeature(retval, 0, sub, &rpc->iffeature[rpc->iffeature_size], unres);
            rpc->iffeature_size++;
            if (r) {
                goto error;
            }
        }
    }

    lyp_reduce_ext_list(&retval->ext, retval->ext_size, c_ext + retval->ext_size);

    /* last part - process data nodes */
    LY_TREE_FOR_SAFE(root.child, next, sub) {
        if (!strcmp(sub->name, "grouping")) {
            node = read_yin_grouping(module, retval, sub, options, unres);
        } else if (!strcmp(sub->name, "input") || !strcmp(sub->name, "output")) {
            node = read_yin_input_output(module, retval, sub, options, unres);
        }
        if (!node) {
            goto error;
        }

        lyxml_free(ctx, sub);
    }

    return retval;

error:
    lys_node_free(retval, NULL, 0);
    while (root.child) {
        lyxml_free(ctx, root.child);
    }
    return NULL;
}