read_yin_leaflist(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin, int options,
                  struct unres_schema *unres)
{
    struct ly_ctx *ctx = module->ctx;
    struct lys_node *retval;
    struct lys_node_leaflist *llist;
    struct lyxml_elem *sub, *next;
    const char *value;
    char *endptr;
    unsigned long val;
    int r, has_type = 0;
    int c_must = 0, c_ftrs = 0, c_dflt = 0, c_ext = 0;
    int f_ordr = 0, f_min = 0, f_max = 0;
    void *reallocated;

    llist = calloc(1, sizeof *llist);
    LY_CHECK_ERR_RETURN(!llist, LOGMEM(ctx), NULL);

    llist->nodetype = LYS_LEAFLIST;
    llist->prev = (struct lys_node *)llist;
    retval = (struct lys_node *)llist;

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
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions", "leaf-list", error);
            c_ext++;
            continue;
        } else if (!strcmp(sub->name, "type")) {
            if (has_type) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            /* HACK for unres */
            llist->type.der = (struct lys_tpdf *)sub;
            llist->type.parent = (struct lys_tpdf *)llist;
            /* postpone type resolution when if-feature parsing is done since we need
             * if-feature for check_leafref_features() */
            has_type = 1;
        } else if (!strcmp(sub->name, "units")) {
            if (llist->units) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            GETVAL(ctx, value, sub, "name");
            llist->units = lydict_insert(ctx, value, strlen(value));

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_UNITS, 0, unres)) {
                goto error;
            }
        } else if (!strcmp(sub->name, "ordered-by")) {
            if (f_ordr) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }
            /* just checking the flags in llist is not sufficient, we would
             * allow multiple ordered-by statements with the "system" value
             */
            f_ordr = 1;

            if (llist->flags & LYS_CONFIG_R) {
                /* RFC 6020, 7.7.5 - ignore ordering when the list represents
                 * state data
                 */
                lyxml_free(ctx, sub);
                continue;
            }

            GETVAL(ctx, value, sub, "value");
            if (!strcmp(value, "user")) {
                llist->flags |= LYS_USERORDERED;
            } else if (strcmp(value, "system")) {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                goto error;
            } /* else system is the default value, so we can ignore it */

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_ORDEREDBY, 0, unres)) {
                goto error;
            }
        } else if (!strcmp(sub->name, "must")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_must, llist->must_size, "musts", "leaf-list", error);
            c_must++;
            continue;
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "if-features", "leaf-list", error);
            c_ftrs++;
            continue;
        } else if ((module->version >= 2) && !strcmp(sub->name, "default")) {
            /* read the default's extension instances */
            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_DEFAULT, c_dflt, unres)) {
                goto error;
            }

            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_dflt, llist->dflt_size, "defaults", "leaf-list", error);
            c_dflt++;
            continue;

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
            endptr = NULL;
            val = strtoul(value, &endptr, 10);
            if (*endptr || value[0] == '-' || errno || val > UINT32_MAX) {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                goto error;
            }
            llist->min = (uint32_t) val;
            if (llist->max && (llist->min > llist->max)) {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "\"min-elements\" is bigger than \"max-elements\".");
                goto error;
            }

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_MIN, 0, unres)) {
                goto error;
            }
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
                llist->max = 0;
            } else {
                /* convert it to uint32_t */
                errno = 0;
                endptr = NULL;
                val = strtoul(value, &endptr, 10);
                if (*endptr || value[0] == '-' || errno || val == 0 || val > UINT32_MAX) {
                    LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                    goto error;
                }
                llist->max = (uint32_t) val;
                if (llist->min > llist->max) {
                    LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                    LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "\"max-elements\" is smaller than \"min-elements\".");
                    goto error;
                }
            }

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_MAX, 0, unres)) {
                goto error;
            }
        } else if (!strcmp(sub->name, "when")) {
            if (llist->when) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }

            llist->when = read_yin_when(module, sub, unres);
            if (!llist->when) {
                goto error;
            }
        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }

        /* do not free sub, it could have been unlinked and stored in unres */
    }

    /* check constraints */
    if (!has_type) {
        LOGVAL(ctx, LYE_MISSCHILDSTMT, LY_VLOG_LYS, retval, "type", yin->name);
        goto error;
    }

    /* middle part - process nodes with cardinality of 0..n */
    if (c_must) {
        llist->must = calloc(c_must, sizeof *llist->must);
        LY_CHECK_ERR_GOTO(!llist->must, LOGMEM(ctx), error);
    }
    if (c_ftrs) {
        llist->iffeature = calloc(c_ftrs, sizeof *llist->iffeature);
        LY_CHECK_ERR_GOTO(!llist->iffeature, LOGMEM(ctx), error);
    }
    if (c_dflt) {
        llist->dflt = calloc(c_dflt, sizeof *llist->dflt);
        LY_CHECK_ERR_GOTO(!llist->dflt, LOGMEM(ctx), error);
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
            r = fill_yin_must(module, sub, &llist->must[llist->must_size], unres);
            llist->must_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "if-feature")) {
            r = fill_yin_iffeature(retval, 0, sub, &llist->iffeature[llist->iffeature_size], unres);
            llist->iffeature_size++;
            if (r) {
                goto error;
            }
        } else if (!strcmp(sub->name, "default")) {
            GETVAL(ctx, value, sub, "value");

            /* check for duplicity in case of configuration data,
             * in case of status data duplicities are allowed */
            if (llist->flags & LYS_CONFIG_W) {
                for (r = 0; r < llist->dflt_size; r++) {
                    if (ly_strequal(llist->dflt[r], value, 1)) {
                        LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, "default");
                        LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "Duplicated default value \"%s\".", value);
                        goto error;
                    }
                }
            }
            llist->dflt[llist->dflt_size++] = lydict_insert(ctx, value, strlen(value));
        }
    }

    lyp_reduce_ext_list(&retval->ext, retval->ext_size, c_ext + retval->ext_size);

    /* finalize type parsing */
    if (unres_schema_add_node(module, unres, &llist->type, UNRES_TYPE_DER, retval) == -1) {
        llist->type.der = NULL;
        goto error;
    }

    if (llist->dflt_size && llist->min) {
        LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, retval, "min-elements", "leaf-list");
        LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL,
               "The \"min-elements\" statement with non-zero value is forbidden on leaf-lists with the \"default\" statement.");
        goto error;
    }

    /* check default value (if not defined, there still could be some restrictions
     * that need to be checked against a default value from a derived type) */
    for (r = 0; r < llist->dflt_size; r++) {
        if (!(ctx->models.flags & LY_CTX_TRUSTED) &&
                (unres_schema_add_node(module, unres, &llist->type, UNRES_TYPE_DFLT,
                                       (struct lys_node *)(&llist->dflt[r])) == -1)) {
            goto error;
        }
    }

    /* check XPath dependencies */
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && (llist->when || llist->must)) {
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