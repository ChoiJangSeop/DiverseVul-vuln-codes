read_yin_leaf(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin, int options,
              struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lys_node *retval;
    struct lys_node_leaf *leaf;
    struct lyxml_elem *sub, *next;
    const char *value;
    int r, has_type = 0;
    int c_must = 0, c_ftrs = 0, f_mand = 0, c_ext = 0;
    void *reallocated;

    leaf = calloc(1, sizeof *leaf);
    LY_CHECK_ERR_RETURN(!leaf, LOGMEM(ctx), NULL);

    leaf->nodetype = LYS_LEAF;
    leaf->prev = (struct lys_node *)leaf;
    retval = (struct lys_node *)leaf;

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

    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions", "leaf", error);
            c_ext++;
            continue;
        } else if (!strcmp(sub->name, "type")) {
            if (has_type) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            /* HACK for unres */
            leaf->type.der = (struct lys_tpdf *)sub;
            leaf->type.parent = (struct lys_tpdf *)leaf;
            /* postpone type resolution when if-feature parsing is done since we need
             * if-feature for check_leafref_features() */
            has_type = 1;
        } else if (!strcmp(sub->name, "default")) {
            if (leaf->dflt) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            GETVAL(ctx, value, sub, "value");
            leaf->dflt = lydict_insert(ctx, value, strlen(value));

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_DEFAULT, 0, unres)) {
                goto error;
            }
        } else if (!strcmp(sub->name, "units")) {
            if (leaf->units) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            GETVAL(ctx, value, sub, "name");
            leaf->units = lydict_insert(ctx, value, strlen(value));

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_UNITS, 0, unres)) {
                goto error;
            }
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
                leaf->flags |= LYS_MAND_TRUE;
            } else if (!strcmp(value, "false")) {
                leaf->flags |= LYS_MAND_FALSE;
            } else {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                goto error;
            }                   /* else false is the default value, so we can ignore it */

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_MANDATORY, 0, unres)) {
                goto error;
            }
        } else if (!strcmp(sub->name, "when")) {
            if (leaf->when) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }

            leaf->when = read_yin_when(module, sub, unres);
            if (!leaf->when) {
                goto error;
            }

        } else if (!strcmp(sub->name, "must")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_must, leaf->must_size, "musts", "leaf", error);
            c_must++;
            continue;
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "musts", "leaf", error);
            c_ftrs++;
            continue;

        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }

        /* do not free sub, it could have been unlinked and stored in unres */
    }

    /* check mandatory parameters */
    if (!has_type) {
        LOGVAL(ctx, LYE_MISSCHILDSTMT, LY_VLOG_LYS, retval, "type", yin->name);
        goto error;
    }
    if (leaf->dflt && (leaf->flags & LYS_MAND_TRUE)) {
        LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, retval, "mandatory", "leaf");
        LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL,
               "The \"mandatory\" statement is forbidden on leaf with the \"default\" statement.");
        goto error;
    }

    /* middle part - process nodes with cardinality of 0..n */
    if (c_must) {
        leaf->must = calloc(c_must, sizeof *leaf->must);
        LY_CHECK_ERR_GOTO(!leaf->must, LOGMEM(ctx), error);
    }
    if (c_ftrs) {
        leaf->iffeature = calloc(c_ftrs, sizeof *leaf->iffeature);
        LY_CHECK_ERR_GOTO(!leaf->iffeature, LOGMEM(ctx), error);
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
            r = fill_yin_must(module, sub, &leaf->must[leaf->must_size], unres);
            leaf->must_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "if-feature")) {
            r = fill_yin_iffeature(retval, 0, sub, &leaf->iffeature[leaf->iffeature_size], unres);
            leaf->iffeature_size++;
            if (r) {
                goto error;
            }
        }
    }

    lyp_reduce_ext_list(&retval->ext, retval->ext_size, c_ext + retval->ext_size);

    /* finalize type parsing */
    if (unres_schema_add_node(module, unres, &leaf->type, UNRES_TYPE_DER, retval) == -1) {
        leaf->type.der = NULL;
        goto error;
    }

    /* check default value (if not defined, there still could be some restrictions
     * that need to be checked against a default value from a derived type) */
    if (!(ctx->models.flags & LY_CTX_TRUSTED) &&
            (unres_schema_add_node(module, unres, &leaf->type, UNRES_TYPE_DFLT,
                                   (struct lys_node *)(&leaf->dflt)) == -1)) {
        goto error;
    }

    /* check XPath dependencies */
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && (leaf->when || leaf->must)) {
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
    lys_node_free(ctx, retval, NULL, 0);
    return NULL;
}