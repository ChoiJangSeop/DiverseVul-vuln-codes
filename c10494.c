lys_node_dup_recursion(struct lys_module *module, struct lys_node *parent, const struct lys_node *node,
                       struct unres_schema *unres, int shallow, int finalize)
{
    struct lys_node *retval = NULL, *iter, *p;
    struct ly_ctx *ctx = module->ctx;
    enum int_log_opts prev_ilo;
    int i, j, rc;
    unsigned int size, size1, size2;
    struct unres_list_uniq *unique_info;
    uint16_t flags;

    struct lys_node_container *cont = NULL;
    struct lys_node_container *cont_orig = (struct lys_node_container *)node;
    struct lys_node_choice *choice = NULL;
    struct lys_node_choice *choice_orig = (struct lys_node_choice *)node;
    struct lys_node_leaf *leaf = NULL;
    struct lys_node_leaf *leaf_orig = (struct lys_node_leaf *)node;
    struct lys_node_leaflist *llist = NULL;
    struct lys_node_leaflist *llist_orig = (struct lys_node_leaflist *)node;
    struct lys_node_list *list = NULL;
    struct lys_node_list *list_orig = (struct lys_node_list *)node;
    struct lys_node_anydata *any = NULL;
    struct lys_node_anydata *any_orig = (struct lys_node_anydata *)node;
    struct lys_node_uses *uses = NULL;
    struct lys_node_uses *uses_orig = (struct lys_node_uses *)node;
    struct lys_node_rpc_action *rpc = NULL;
    struct lys_node_inout *io = NULL;
    struct lys_node_notif *ntf = NULL;
    struct lys_node_case *cs = NULL;
    struct lys_node_case *cs_orig = (struct lys_node_case *)node;

    /* we cannot just duplicate memory since the strings are stored in
     * dictionary and we need to update dictionary counters.
     */

    switch (node->nodetype) {
    case LYS_CONTAINER:
        cont = calloc(1, sizeof *cont);
        retval = (struct lys_node *)cont;
        break;

    case LYS_CHOICE:
        choice = calloc(1, sizeof *choice);
        retval = (struct lys_node *)choice;
        break;

    case LYS_LEAF:
        leaf = calloc(1, sizeof *leaf);
        retval = (struct lys_node *)leaf;
        break;

    case LYS_LEAFLIST:
        llist = calloc(1, sizeof *llist);
        retval = (struct lys_node *)llist;
        break;

    case LYS_LIST:
        list = calloc(1, sizeof *list);
        /* copy keys now so that when adding children, it can be properly checked in this parent */
        list->keys = calloc(list_orig->keys_size, sizeof *list->keys);
        LY_CHECK_ERR_GOTO(!list->keys, LOGMEM(ctx), error);
        list->keys_size = list_orig->keys_size;

        retval = (struct lys_node *)list;
        break;

    case LYS_ANYXML:
    case LYS_ANYDATA:
        any = calloc(1, sizeof *any);
        retval = (struct lys_node *)any;
        break;

    case LYS_USES:
        uses = calloc(1, sizeof *uses);
        retval = (struct lys_node *)uses;
        break;

    case LYS_CASE:
        cs = calloc(1, sizeof *cs);
        retval = (struct lys_node *)cs;
        break;

    case LYS_RPC:
    case LYS_ACTION:
        rpc = calloc(1, sizeof *rpc);
        retval = (struct lys_node *)rpc;
        break;

    case LYS_INPUT:
    case LYS_OUTPUT:
        io = calloc(1, sizeof *io);
        retval = (struct lys_node *)io;
        break;

    case LYS_NOTIF:
        ntf = calloc(1, sizeof *ntf);
        retval = (struct lys_node *)ntf;
        break;

    default:
        LOGINT(ctx);
        goto error;
    }
    LY_CHECK_ERR_RETURN(!retval, LOGMEM(ctx), NULL);

    /*
     * duplicate generic part of the structure
     */
    retval->name = lydict_insert(ctx, node->name, 0);
    retval->dsc = lydict_insert(ctx, node->dsc, 0);
    retval->ref = lydict_insert(ctx, node->ref, 0);
    retval->flags = node->flags;

    retval->module = module;
    retval->nodetype = node->nodetype;

    retval->prev = retval;

    /* copying unresolved extensions is not supported */
    if (unres_schema_find(unres, -1, (void *)&node->ext, UNRES_EXT) == -1) {
        retval->ext_size = node->ext_size;
        if (lys_ext_dup(ctx, module, node->ext, node->ext_size, retval, LYEXT_PAR_NODE, &retval->ext, shallow, unres)) {
            goto error;
        }
    }

    if (node->iffeature_size) {
        retval->iffeature_size = node->iffeature_size;
        retval->iffeature = calloc(retval->iffeature_size, sizeof *retval->iffeature);
        LY_CHECK_ERR_GOTO(!retval->iffeature, LOGMEM(ctx), error);
    }

    if (!shallow) {
        for (i = 0; i < node->iffeature_size; ++i) {
            resolve_iffeature_getsizes(&node->iffeature[i], &size1, &size2);
            if (size1) {
                /* there is something to duplicate */

                /* duplicate compiled expression */
                size = (size1 / 4) + ((size1 % 4) ? 1 : 0);
                retval->iffeature[i].expr = malloc(size * sizeof *retval->iffeature[i].expr);
                LY_CHECK_ERR_GOTO(!retval->iffeature[i].expr, LOGMEM(ctx), error);
                memcpy(retval->iffeature[i].expr, node->iffeature[i].expr, size * sizeof *retval->iffeature[i].expr);

                /* list of feature pointer must be updated to point to the resulting tree */
                retval->iffeature[i].features = calloc(size2, sizeof *retval->iffeature[i].features);
                LY_CHECK_ERR_GOTO(!retval->iffeature[i].features, LOGMEM(ctx); free(retval->iffeature[i].expr), error);

                for (j = 0; (unsigned int)j < size2; j++) {
                    rc = unres_schema_dup(module, unres, &node->iffeature[i].features[j], UNRES_IFFEAT,
                                          &retval->iffeature[i].features[j]);
                    if (rc == EXIT_FAILURE) {
                        /* feature is resolved in origin, so copy it
                         * - duplication is used for instantiating groupings
                         * and if-feature inside grouping is supposed to be
                         * resolved inside the original grouping, so we want
                         * to keep pointers to features from the grouping
                         * context */
                        retval->iffeature[i].features[j] = node->iffeature[i].features[j];
                    } else if (rc == -1) {
                        goto error;
                    } /* else unres was duplicated */
                }
            }

            /* duplicate if-feature's extensions */
            retval->iffeature[i].ext_size = node->iffeature[i].ext_size;
            if (lys_ext_dup(ctx, module, node->iffeature[i].ext, node->iffeature[i].ext_size,
                            &retval->iffeature[i], LYEXT_PAR_IFFEATURE, &retval->iffeature[i].ext, shallow, unres)) {
                goto error;
            }
        }

        /* inherit config flags */
        p = parent;
        do {
            for (iter = p; iter && (iter->nodetype == LYS_USES); iter = iter->parent);
        } while (iter && iter->nodetype == LYS_AUGMENT && (p = ((struct lys_node_augment *)iter)->target));
        if (iter) {
            flags = iter->flags & LYS_CONFIG_MASK;
        } else {
            /* default */
            flags = LYS_CONFIG_W;
        }

        switch (finalize) {
        case 1:
            /* inherit config flags */
            if (retval->flags & LYS_CONFIG_SET) {
                /* skip nodes with an explicit config value */
                if ((flags & LYS_CONFIG_R) && (retval->flags & LYS_CONFIG_W)) {
                    LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, retval, "true", "config");
                    LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "State nodes cannot have configuration nodes as children.");
                    goto error;
                }
                break;
            }

            if (retval->nodetype != LYS_USES) {
                retval->flags = (retval->flags & ~LYS_CONFIG_MASK) | flags;
            }

            /* inherit status */
            if ((parent->flags & LYS_STATUS_MASK) > (retval->flags & LYS_STATUS_MASK)) {
                /* but do it only in case the parent has a stonger status */
                retval->flags &= ~LYS_STATUS_MASK;
                retval->flags |= (parent->flags & LYS_STATUS_MASK);
            }
            break;
        case 2:
            /* erase config flags */
            retval->flags &= ~LYS_CONFIG_MASK;
            retval->flags &= ~LYS_CONFIG_SET;
            break;
        }

        /* connect it to the parent */
        if (lys_node_addchild(parent, retval->module, retval, 0)) {
            goto error;
        }

        /* go recursively */
        if (!(node->nodetype & (LYS_LEAF | LYS_LEAFLIST))) {
            LY_TREE_FOR(node->child, iter) {
                if (iter->nodetype & LYS_GROUPING) {
                    /* do not instantiate groupings */
                    continue;
                }
                if (!lys_node_dup_recursion(module, retval, iter, unres, 0, finalize)) {
                    goto error;
                }
            }
        }
    } else {
        if (node->iffeature_size) {
            memcpy(retval->iffeature, node->iffeature, retval->iffeature_size * sizeof *retval->iffeature);
        }
    }

    /*
     * duplicate specific part of the structure
     */
    switch (node->nodetype) {
    case LYS_CONTAINER:
        if (cont_orig->when) {
            cont->when = lys_when_dup(module, cont_orig->when, shallow, unres);
            LY_CHECK_GOTO(!cont->when, error);
        }
        cont->presence = lydict_insert(ctx, cont_orig->presence, 0);

        if (cont_orig->must) {
            cont->must = lys_restr_dup(module, cont_orig->must, cont_orig->must_size, shallow, unres);
            LY_CHECK_GOTO(!cont->must, error);
            cont->must_size = cont_orig->must_size;
        }

        /* typedefs are not needed in instantiated grouping, nor the deviation's shallow copy */

        break;
    case LYS_CHOICE:
        if (choice_orig->when) {
            choice->when = lys_when_dup(module, choice_orig->when, shallow, unres);
            LY_CHECK_GOTO(!choice->when, error);
        }

        if (!shallow) {
            if (choice_orig->dflt) {
                rc = lys_get_sibling(choice->child, lys_node_module(retval)->name, 0, choice_orig->dflt->name, 0,
                                            LYS_ANYDATA | LYS_CASE | LYS_CONTAINER | LYS_LEAF | LYS_LEAFLIST | LYS_LIST,
                                            (const struct lys_node **)&choice->dflt);
                if (rc) {
                    if (rc == EXIT_FAILURE) {
                        LOGINT(ctx);
                    }
                    goto error;
                }
            } else {
                /* useless to check return value, we don't know whether
                * there really wasn't any default defined or it just hasn't
                * been resolved, we just hope for the best :)
                */
                unres_schema_dup(module, unres, choice_orig, UNRES_CHOICE_DFLT, choice);
            }
        } else {
            choice->dflt = choice_orig->dflt;
        }
        break;

    case LYS_LEAF:
        if (lys_type_dup(module, retval, &(leaf->type), &(leaf_orig->type), lys_ingrouping(retval), shallow, unres)) {
            goto error;
        }
        leaf->units = lydict_insert(module->ctx, leaf_orig->units, 0);

        if (leaf_orig->dflt) {
            /* transform into JSON format, may not be possible later */
            ly_ilo_change(NULL, ILO_IGNORE, &prev_ilo, NULL);
            leaf->dflt = transform_schema2json(lys_main_module(leaf_orig->module), leaf_orig->dflt);
            ly_ilo_restore(NULL, prev_ilo, NULL, 0);
            if (!leaf->dflt) {
                /* invalid identityref format or it was already transformed, so ignore the error here */
                leaf->dflt = lydict_insert(ctx, leaf_orig->dflt, 0);
            }
        }

        if (leaf_orig->must) {
            leaf->must = lys_restr_dup(module, leaf_orig->must, leaf_orig->must_size, shallow, unres);
            LY_CHECK_GOTO(!leaf->must, error);
            leaf->must_size = leaf_orig->must_size;
        }

        if (leaf_orig->when) {
            leaf->when = lys_when_dup(module, leaf_orig->when, shallow, unres);
            LY_CHECK_GOTO(!leaf->when, error);
        }
        break;

    case LYS_LEAFLIST:
        if (lys_type_dup(module, retval, &(llist->type), &(llist_orig->type), lys_ingrouping(retval), shallow, unres)) {
            goto error;
        }
        llist->units = lydict_insert(module->ctx, llist_orig->units, 0);

        llist->min = llist_orig->min;
        llist->max = llist_orig->max;

        if (llist_orig->must) {
            llist->must = lys_restr_dup(module, llist_orig->must, llist_orig->must_size, shallow, unres);
            LY_CHECK_GOTO(!llist->must, error);
            llist->must_size = llist_orig->must_size;
        }

        if (llist_orig->dflt) {
            llist->dflt = malloc(llist_orig->dflt_size * sizeof *llist->dflt);
            LY_CHECK_ERR_GOTO(!llist->dflt, LOGMEM(ctx), error);
            llist->dflt_size = llist_orig->dflt_size;

            for (i = 0; i < llist->dflt_size; i++) {
                llist->dflt[i] = lydict_insert(ctx, llist_orig->dflt[i], 0);
            }
        }

        if (llist_orig->when) {
            llist->when = lys_when_dup(module, llist_orig->when, shallow, unres);
        }
        break;

    case LYS_LIST:
        list->min = list_orig->min;
        list->max = list_orig->max;

        if (list_orig->must) {
            list->must = lys_restr_dup(module, list_orig->must, list_orig->must_size, shallow, unres);
            LY_CHECK_GOTO(!list->must, error);
            list->must_size = list_orig->must_size;
        }

        /* typedefs are not needed in instantiated grouping, nor the deviation's shallow copy */

        if (list_orig->keys_size) {
            /* already done */
            /*list->keys = calloc(list_orig->keys_size, sizeof *list->keys);
            LY_CHECK_ERR_GOTO(!list->keys, LOGMEM(ctx), error);
            list->keys_size = list_orig->keys_size;*/
            list->keys_str = lydict_insert(ctx, list_orig->keys_str, 0);

            if (!shallow) {
                if (unres_schema_add_node(module, unres, list, UNRES_LIST_KEYS, NULL) == -1) {
                    goto error;
                }
            } else {
                memcpy(list->keys, list_orig->keys, list_orig->keys_size * sizeof *list->keys);
            }
        }

        if (list_orig->unique) {
            list->unique = malloc(list_orig->unique_size * sizeof *list->unique);
            LY_CHECK_ERR_GOTO(!list->unique, LOGMEM(ctx), error);
            list->unique_size = list_orig->unique_size;

            for (i = 0; i < list->unique_size; ++i) {
                list->unique[i].expr = malloc(list_orig->unique[i].expr_size * sizeof *list->unique[i].expr);
                LY_CHECK_ERR_GOTO(!list->unique[i].expr, LOGMEM(ctx), error);
                list->unique[i].expr_size = list_orig->unique[i].expr_size;
                for (j = 0; j < list->unique[i].expr_size; j++) {
                    list->unique[i].expr[j] = lydict_insert(ctx, list_orig->unique[i].expr[j], 0);

                    /* if it stays in unres list, duplicate it also there */
                    unique_info = malloc(sizeof *unique_info);
                    LY_CHECK_ERR_GOTO(!unique_info, LOGMEM(ctx), error);
                    unique_info->list = (struct lys_node *)list;
                    unique_info->expr = list->unique[i].expr[j];
                    unique_info->trg_type = &list->unique[i].trg_type;
                    unres_schema_dup(module, unres, &list_orig, UNRES_LIST_UNIQ, unique_info);
                }
            }
        }

        if (list_orig->when) {
            list->when = lys_when_dup(module, list_orig->when, shallow, unres);
            LY_CHECK_GOTO(!list->when, error);
        }
        break;

    case LYS_ANYXML:
    case LYS_ANYDATA:
        if (any_orig->must) {
            any->must = lys_restr_dup(module, any_orig->must, any_orig->must_size, shallow, unres);
            LY_CHECK_GOTO(!any->must, error);
            any->must_size = any_orig->must_size;
        }

        if (any_orig->when) {
            any->when = lys_when_dup(module, any_orig->when, shallow, unres);
            LY_CHECK_GOTO(!any->when, error);
        }
        break;

    case LYS_USES:
        uses->grp = uses_orig->grp;

        if (uses_orig->when) {
            uses->when = lys_when_dup(module, uses_orig->when, shallow, unres);
            LY_CHECK_GOTO(!uses->when, error);
        }
        /* it is not needed to duplicate refine, nor augment. They are already applied to the uses children */
        break;

    case LYS_CASE:
        if (cs_orig->when) {
            cs->when = lys_when_dup(module, cs_orig->when, shallow, unres);
            LY_CHECK_GOTO(!cs->when, error);
        }
        break;

    case LYS_ACTION:
    case LYS_RPC:
    case LYS_INPUT:
    case LYS_OUTPUT:
    case LYS_NOTIF:
        /* typedefs are not needed in instantiated grouping, nor the deviation's shallow copy */
        break;

    default:
        /* LY_NODE_AUGMENT */
        LOGINT(ctx);
        goto error;
    }

    return retval;

error:
    lys_node_free(retval, NULL, 0);
    return NULL;
}