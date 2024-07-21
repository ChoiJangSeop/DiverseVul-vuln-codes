read_yin_anydata(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin, LYS_NODE type,
                 int options, struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lys_node *retval;
    struct lys_node_anydata *anyxml;
    struct lyxml_elem *sub, *next;
    const char *value;
    int r;
    int f_mand = 0;
    int c_must = 0, c_ftrs = 0, c_ext = 0;
    void *reallocated;

    anyxml = calloc(1, sizeof *anyxml);
    LY_CHECK_ERR_RETURN(!anyxml, LOGMEM(ctx), NULL);

    anyxml->nodetype = type;
    anyxml->prev = (struct lys_node *)anyxml;
    retval = (struct lys_node *)anyxml;

    if (read_yin_common(module, parent, retval, LYEXT_PAR_NODE, yin,
            OPT_IDENT | OPT_MODULE | ((options & LYS_PARSE_OPT_CFG_IGNORE) ? OPT_CFG_IGNORE :
            (options & LYS_PARSE_OPT_CFG_NOINHERIT) ? OPT_CFG_PARSE : OPT_CFG_PARSE | OPT_CFG_INHERIT), unres)) {
        goto error;
    }

    LOGDBG(LY_LDGYIN, "parsing %s statement \"%s\"", yin->name, retval->name);

    /* insert the node into the schema tree */
    if (lys_node_addchild(parent, lys_main_module(module), retval, options)) {
        goto error;
    }

    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions", "anydata", error);
            c_ext++;
        } else if (!strcmp(sub->name, "mandatory")) {
            if (f_mand) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            /* just checking the flags in leaf is not sufficient, we would allow
             * multiple mandatory statements with the "false" value
             */
            f_mand = 1;

            GETVAL(ctx, value, sub, "value");
            if (!strcmp(value, "true")) {
                anyxml->flags |= LYS_MAND_TRUE;
            } else if (!strcmp(value, "false")) {
                anyxml->flags |= LYS_MAND_FALSE;
            } else {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                goto error;
            }
            /* else false is the default value, so we can ignore it */

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_MANDATORY, 0, unres)) {
                goto error;
            }
            lyxml_free(ctx, sub);
        } else if (!strcmp(sub->name, "when")) {
            if (anyxml->when) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }

            anyxml->when = read_yin_when(module, sub, unres);
            if (!anyxml->when) {
                lyxml_free(ctx, sub);
                goto error;
            }
            lyxml_free(ctx, sub);
        } else if (!strcmp(sub->name, "must")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_must, anyxml->must_size, "musts", "anydata", error);
            c_must++;
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "if-features", "anydata", error);
            c_ftrs++;

        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }
    }

    /* middle part - process nodes with cardinality of 0..n */
    if (c_must) {
        anyxml->must = calloc(c_must, sizeof *anyxml->must);
        LY_CHECK_ERR_GOTO(!anyxml->must, LOGMEM(ctx), error);
    }
    if (c_ftrs) {
        anyxml->iffeature = calloc(c_ftrs, sizeof *anyxml->iffeature);
        LY_CHECK_ERR_GOTO(!anyxml->iffeature, LOGMEM(ctx), error);
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
        } else if (!strcmp(sub->name, "must")) {
            r = fill_yin_must(module, sub, &anyxml->must[anyxml->must_size], unres);
            anyxml->must_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "if-feature")) {
            r = fill_yin_iffeature(retval, 0, sub, &anyxml->iffeature[anyxml->iffeature_size], unres);
            anyxml->iffeature_size++;
            if (r) {
                goto error;
            }
        }
    }

    lyp_reduce_ext_list(&retval->ext, retval->ext_size, c_ext + retval->ext_size);

    /* check XPath dependencies */
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && (anyxml->when || anyxml->must)) {
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
            break;
        }
    }

    return retval;

error:
    lys_node_free(retval, NULL, 0);
    return NULL;
}