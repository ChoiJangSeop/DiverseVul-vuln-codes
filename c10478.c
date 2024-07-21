read_yin_choice(struct lys_module *module, struct lys_node *parent, struct lyxml_elem *yin, int options,
                struct unres_schema *unres)
{
    struct lyxml_elem *sub, *next, *dflt = NULL;
    struct ly_ctx *const ctx = module->ctx;
    struct lys_node *retval, *node = NULL;
    struct lys_node_choice *choice;
    const char *value;
    int f_mand = 0, c_ftrs = 0, c_ext = 0, ret;
    void *reallocated;

    choice = calloc(1, sizeof *choice);
    LY_CHECK_ERR_RETURN(!choice, LOGMEM(ctx), NULL);

    choice->nodetype = LYS_CHOICE;
    choice->prev = (struct lys_node *)choice;
    retval = (struct lys_node *)choice;

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

    /* process choice's specific children */
    LY_TREE_FOR_SAFE(yin->child, next, sub) {
        if (strcmp(sub->ns->value, LY_NSYIN)) {
            /* extension */
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ext, retval->ext_size, "extensions", "choice", error);
            c_ext++;
            /* keep it for later processing, skip lyxml_free() */
            continue;
        } else if (!strcmp(sub->name, "container")) {
            if (!(node = read_yin_container(module, retval, sub, options, unres))) {
                goto error;
            }
        } else if (!strcmp(sub->name, "leaf-list")) {
            if (!(node = read_yin_leaflist(module, retval, sub, options, unres))) {
                goto error;
            }
        } else if (!strcmp(sub->name, "leaf")) {
            if (!(node = read_yin_leaf(module, retval, sub, options, unres))) {
                goto error;
            }
        } else if (!strcmp(sub->name, "list")) {
            if (!(node = read_yin_list(module, retval, sub, options, unres))) {
                goto error;
            }
        } else if (!strcmp(sub->name, "case")) {
            if (!(node = read_yin_case(module, retval, sub, options, unres))) {
                goto error;
            }
        } else if (!strcmp(sub->name, "anyxml")) {
            if (!(node = read_yin_anydata(module, retval, sub, LYS_ANYXML, options, unres))) {
                goto error;
            }
        } else if (!strcmp(sub->name, "anydata")) {
            if (!(node = read_yin_anydata(module, retval, sub, LYS_ANYDATA, options, unres))) {
                goto error;
            }
        } else if (!strcmp(sub->name, "default")) {
            if (dflt) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_DEFAULT, 0, unres)) {
                goto error;
            }

            dflt = sub;
            lyxml_unlink_elem(ctx, dflt, 0);
            continue;
            /* skip lyxml_free() at the end of the loop, the sub node is processed later as dflt */

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
                choice->flags |= LYS_MAND_TRUE;
            } else if (!strcmp(value, "false")) {
                choice->flags |= LYS_MAND_FALSE;
            } else {
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, value, sub->name);
                goto error;
            }                   /* else false is the default value, so we can ignore it */

            if (lyp_yin_parse_subnode_ext(module, retval, LYEXT_PAR_NODE, sub, LYEXT_SUBSTMT_MANDATORY, 0, unres)) {
                goto error;
            }
        } else if (!strcmp(sub->name, "when")) {
            if (choice->when) {
                LOGVAL(ctx, LYE_TOOMANY, LY_VLOG_LYS, retval, sub->name, yin->name);
                goto error;
            }

            choice->when = read_yin_when(module, sub, unres);
            if (!choice->when) {
                goto error;
            }
        } else if (!strcmp(sub->name, "if-feature")) {
            YIN_CHECK_ARRAY_OVERFLOW_GOTO(ctx, c_ftrs, retval->iffeature_size, "if-features", "choice", error);
            c_ftrs++;

            /* skip lyxml_free() at the end of the loop, the sub node is processed later */
            continue;
        } else if (module->version >= 2 && !strcmp(sub->name, "choice")) {
            if (!(node = read_yin_choice(module, retval, sub, options, unres))) {
                goto error;
            }
        } else {
            LOGVAL(ctx, LYE_INSTMT, LY_VLOG_LYS, retval, sub->name);
            goto error;
        }

        node = NULL;
        lyxml_free(ctx, sub);
    }

    if (c_ftrs) {
        choice->iffeature = calloc(c_ftrs, sizeof *choice->iffeature);
        LY_CHECK_ERR_GOTO(!choice->iffeature, LOGMEM(ctx), error);
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
            ret = fill_yin_iffeature(retval, 0, sub, &choice->iffeature[choice->iffeature_size], unres);
            choice->iffeature_size++;
            if (ret) {
                goto error;
            }
        }
    }

    lyp_reduce_ext_list(&retval->ext, retval->ext_size, c_ext + retval->ext_size);

    /* check - default is prohibited in combination with mandatory */
    if (dflt && (choice->flags & LYS_MAND_TRUE)) {
        LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, retval, "default", "choice");
        LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "The \"default\" statement is forbidden on choices with \"mandatory\".");
        goto error;
    }

    /* link default with the case */
    if (dflt) {
        GETVAL(ctx, value, dflt, "value");
        if (unres_schema_add_str(module, unres, choice, UNRES_CHOICE_DFLT, value) == -1) {
            goto error;
        }
        lyxml_free(ctx, dflt);
    }

    /* check XPath dependencies */
    if (!(ctx->models.flags & LY_CTX_TRUSTED) && choice->when) {
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
    lyxml_free(ctx, dflt);
    lys_node_free(retval, NULL, 0);
    return NULL;
}