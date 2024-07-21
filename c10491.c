resolve_uses(struct lys_node_uses *uses, struct unres_schema *unres)
{
    struct ly_ctx *ctx = uses->module->ctx; /* shortcut */
    struct lys_node *node = NULL, *next, *iter, **refine_nodes = NULL;
    struct lys_node *node_aux, *parent, *tmp;
    struct lys_node_leaflist *llist;
    struct lys_node_leaf *leaf;
    struct lys_refine *rfn;
    struct lys_restr *must, **old_must;
    struct lys_iffeature *iff, **old_iff;
    int i, j, k, rc;
    uint8_t size, *old_size;
    unsigned int usize, usize1, usize2;

    assert(uses->grp);

    /* check that the grouping is resolved (no unresolved uses inside) */
    assert(!uses->grp->unres_count);

    /* copy the data nodes from grouping into the uses context */
    LY_TREE_FOR(uses->grp->child, node_aux) {
        if (node_aux->nodetype & LYS_GROUPING) {
            /* do not instantiate groupings from groupings */
            continue;
        }
        node = lys_node_dup(uses->module, (struct lys_node *)uses, node_aux, unres, 0);
        if (!node) {
            LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, uses, uses->grp->name, "uses");
            LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "Copying data from grouping failed.");
            goto fail;
        }
        /* test the name of siblings */
        LY_TREE_FOR((uses->parent) ? *lys_child(uses->parent, LYS_USES) : lys_main_module(uses->module)->data, tmp) {
            if (!(tmp->nodetype & (LYS_USES | LYS_GROUPING | LYS_CASE)) && ly_strequal(tmp->name, node_aux->name, 1)) {
                goto fail;
            }
        }
    }

    /* we managed to copy the grouping, the rest must be possible to resolve */

    if (uses->refine_size) {
        refine_nodes = malloc(uses->refine_size * sizeof *refine_nodes);
        LY_CHECK_ERR_GOTO(!refine_nodes, LOGMEM(ctx), fail);
    }

    /* apply refines */
    for (i = 0; i < uses->refine_size; i++) {
        rfn = &uses->refine[i];
        rc = resolve_descendant_schema_nodeid(rfn->target_name, uses->child,
                                              LYS_NO_RPC_NOTIF_NODE | LYS_ACTION | LYS_NOTIF,
                                              0, (const struct lys_node **)&node);
        if (rc || !node) {
            LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, uses, rfn->target_name, "refine");
            goto fail;
        }

        if (rfn->target_type && !(node->nodetype & rfn->target_type)) {
            LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, uses, rfn->target_name, "refine");
            LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "Refine substatements not applicable to the target-node.");
            goto fail;
        }
        refine_nodes[i] = node;

        /* description on any nodetype */
        if (rfn->dsc) {
            lydict_remove(ctx, node->dsc);
            node->dsc = lydict_insert(ctx, rfn->dsc, 0);
        }

        /* reference on any nodetype */
        if (rfn->ref) {
            lydict_remove(ctx, node->ref);
            node->ref = lydict_insert(ctx, rfn->ref, 0);
        }

        /* config on any nodetype,
         * in case of notification or rpc/action, the config is not applicable (there is no config status) */
        if ((rfn->flags & LYS_CONFIG_MASK) && (node->flags & LYS_CONFIG_MASK)) {
            node->flags &= ~LYS_CONFIG_MASK;
            node->flags |= (rfn->flags & LYS_CONFIG_MASK);
        }

        /* default value ... */
        if (rfn->dflt_size) {
            if (node->nodetype == LYS_LEAF) {
                /* leaf */
                leaf = (struct lys_node_leaf *)node;

                /* replace default value */
                lydict_remove(ctx, leaf->dflt);
                leaf->dflt = lydict_insert(ctx, rfn->dflt[0], 0);

                /* check the default value */
                if (unres_schema_add_node(leaf->module, unres, &leaf->type, UNRES_TYPE_DFLT,
                                          (struct lys_node *)(&leaf->dflt)) == -1) {
                    goto fail;
                }
            } else if (node->nodetype == LYS_LEAFLIST) {
                /* leaf-list */
                llist = (struct lys_node_leaflist *)node;

                /* remove complete set of defaults in target */
                for (j = 0; j < llist->dflt_size; j++) {
                    lydict_remove(ctx, llist->dflt[j]);
                }
                free(llist->dflt);

                /* copy the default set from refine */
                llist->dflt = malloc(rfn->dflt_size * sizeof *llist->dflt);
                LY_CHECK_ERR_GOTO(!llist->dflt, LOGMEM(ctx), fail);
                llist->dflt_size = rfn->dflt_size;
                for (j = 0; j < llist->dflt_size; j++) {
                    llist->dflt[j] = lydict_insert(ctx, rfn->dflt[j], 0);
                }

                /* check default value */
                for (j = 0; j < llist->dflt_size; j++) {
                    if (unres_schema_add_node(llist->module, unres, &llist->type, UNRES_TYPE_DFLT,
                                              (struct lys_node *)(&llist->dflt[j])) == -1) {
                        goto fail;
                    }
                }
            }
        }

        /* mandatory on leaf, anyxml or choice */
        if (rfn->flags & LYS_MAND_MASK) {
            /* remove current value */
            node->flags &= ~LYS_MAND_MASK;

            /* set new value */
            node->flags |= (rfn->flags & LYS_MAND_MASK);

            if (rfn->flags & LYS_MAND_TRUE) {
                /* check if node has default value */
                if ((node->nodetype & LYS_LEAF) && ((struct lys_node_leaf *)node)->dflt) {
                    LOGVAL(ctx, LYE_SPEC, LY_VLOG_LYS, uses,
                           "The \"mandatory\" statement is forbidden on leaf with \"default\".");
                    goto fail;
                }
                if ((node->nodetype & LYS_CHOICE) && ((struct lys_node_choice *)node)->dflt) {
                    LOGVAL(ctx, LYE_SPEC, LY_VLOG_LYS, uses,
                           "The \"mandatory\" statement is forbidden on choices with \"default\".");
                    goto fail;
                }
            }
        }

        /* presence on container */
        if ((node->nodetype & LYS_CONTAINER) && rfn->mod.presence) {
            lydict_remove(ctx, ((struct lys_node_container *)node)->presence);
            ((struct lys_node_container *)node)->presence = lydict_insert(ctx, rfn->mod.presence, 0);
        }

        /* min/max-elements on list or leaf-list */
        if (node->nodetype == LYS_LIST) {
            if (rfn->flags & LYS_RFN_MINSET) {
                ((struct lys_node_list *)node)->min = rfn->mod.list.min;
            }
            if (rfn->flags & LYS_RFN_MAXSET) {
                ((struct lys_node_list *)node)->max = rfn->mod.list.max;
            }
        } else if (node->nodetype == LYS_LEAFLIST) {
            if (rfn->flags & LYS_RFN_MINSET) {
                ((struct lys_node_leaflist *)node)->min = rfn->mod.list.min;
            }
            if (rfn->flags & LYS_RFN_MAXSET) {
                ((struct lys_node_leaflist *)node)->max = rfn->mod.list.max;
            }
        }

        /* must in leaf, leaf-list, list, container or anyxml */
        if (rfn->must_size) {
            switch (node->nodetype) {
            case LYS_LEAF:
                old_size = &((struct lys_node_leaf *)node)->must_size;
                old_must = &((struct lys_node_leaf *)node)->must;
                break;
            case LYS_LEAFLIST:
                old_size = &((struct lys_node_leaflist *)node)->must_size;
                old_must = &((struct lys_node_leaflist *)node)->must;
                break;
            case LYS_LIST:
                old_size = &((struct lys_node_list *)node)->must_size;
                old_must = &((struct lys_node_list *)node)->must;
                break;
            case LYS_CONTAINER:
                old_size = &((struct lys_node_container *)node)->must_size;
                old_must = &((struct lys_node_container *)node)->must;
                break;
            case LYS_ANYXML:
            case LYS_ANYDATA:
                old_size = &((struct lys_node_anydata *)node)->must_size;
                old_must = &((struct lys_node_anydata *)node)->must;
                break;
            default:
                LOGINT(ctx);
                goto fail;
            }

            size = *old_size + rfn->must_size;
            must = realloc(*old_must, size * sizeof *rfn->must);
            LY_CHECK_ERR_GOTO(!must, LOGMEM(ctx), fail);
            for (k = 0, j = *old_size; k < rfn->must_size; k++, j++) {
                must[j].ext_size = rfn->must[k].ext_size;
                lys_ext_dup(ctx, rfn->module, rfn->must[k].ext, rfn->must[k].ext_size, &rfn->must[k], LYEXT_PAR_RESTR,
                            &must[j].ext, 0, unres);
                must[j].expr = lydict_insert(ctx, rfn->must[k].expr, 0);
                must[j].dsc = lydict_insert(ctx, rfn->must[k].dsc, 0);
                must[j].ref = lydict_insert(ctx, rfn->must[k].ref, 0);
                must[j].eapptag = lydict_insert(ctx, rfn->must[k].eapptag, 0);
                must[j].emsg = lydict_insert(ctx, rfn->must[k].emsg, 0);
                must[j].flags = rfn->must[k].flags;
            }

            *old_must = must;
            *old_size = size;

            /* check XPath dependencies again */
            if (unres_schema_add_node(node->module, unres, node, UNRES_XPATH, NULL) == -1) {
                goto fail;
            }
        }

        /* if-feature in leaf, leaf-list, list, container or anyxml */
        if (rfn->iffeature_size) {
            old_size = &node->iffeature_size;
            old_iff = &node->iffeature;

            size = *old_size + rfn->iffeature_size;
            iff = realloc(*old_iff, size * sizeof *rfn->iffeature);
            LY_CHECK_ERR_GOTO(!iff, LOGMEM(ctx), fail);
            *old_iff = iff;

            for (k = 0, j = *old_size; k < rfn->iffeature_size; k++, j++) {
                resolve_iffeature_getsizes(&rfn->iffeature[k], &usize1, &usize2);
                if (usize1) {
                    /* there is something to duplicate */
                    /* duplicate compiled expression */
                    usize = (usize1 / 4) + ((usize1 % 4) ? 1 : 0);
                    iff[j].expr = malloc(usize * sizeof *iff[j].expr);
                    LY_CHECK_ERR_GOTO(!iff[j].expr, LOGMEM(ctx), fail);
                    memcpy(iff[j].expr, rfn->iffeature[k].expr, usize * sizeof *iff[j].expr);

                    /* duplicate list of feature pointers */
                    iff[j].features = malloc(usize2 * sizeof *iff[k].features);
                    LY_CHECK_ERR_GOTO(!iff[j].expr, LOGMEM(ctx), fail);
                    memcpy(iff[j].features, rfn->iffeature[k].features, usize2 * sizeof *iff[j].features);

                    /* duplicate extensions */
                    iff[j].ext_size = rfn->iffeature[k].ext_size;
                    lys_ext_dup(ctx, rfn->module, rfn->iffeature[k].ext, rfn->iffeature[k].ext_size,
                                &rfn->iffeature[k], LYEXT_PAR_IFFEATURE, &iff[j].ext, 0, unres);
                }
                (*old_size)++;
            }
            assert(*old_size == size);
        }
    }

    /* apply augments */
    for (i = 0; i < uses->augment_size; i++) {
        rc = resolve_augment(&uses->augment[i], (struct lys_node *)uses, unres);
        if (rc) {
            goto fail;
        }
    }

    /* check refines */
    for (i = 0; i < uses->refine_size; i++) {
        node = refine_nodes[i];
        rfn = &uses->refine[i];

        /* config on any nodetype */
        if ((rfn->flags & LYS_CONFIG_MASK) && (node->flags & LYS_CONFIG_MASK)) {
            for (parent = lys_parent(node); parent && parent->nodetype == LYS_USES; parent = lys_parent(parent));
            if (parent && parent->nodetype != LYS_GROUPING && (parent->flags & LYS_CONFIG_MASK) &&
                    ((parent->flags & LYS_CONFIG_MASK) != (rfn->flags & LYS_CONFIG_MASK)) &&
                    (rfn->flags & LYS_CONFIG_W)) {
                /* setting config true under config false is prohibited */
                LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, uses, "config", "refine");
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL,
                       "changing config from 'false' to 'true' is prohibited while "
                       "the target's parent is still config 'false'.");
                goto fail;
            }

            /* inherit config change to the target children */
            LY_TREE_DFS_BEGIN(node->child, next, iter) {
                if (rfn->flags & LYS_CONFIG_W) {
                    if (iter->flags & LYS_CONFIG_SET) {
                        /* config is set explicitely, go to next sibling */
                        next = NULL;
                        goto nextsibling;
                    }
                } else { /* LYS_CONFIG_R */
                    if ((iter->flags & LYS_CONFIG_SET) && (iter->flags & LYS_CONFIG_W)) {
                        /* error - we would have config data under status data */
                        LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, uses, "config", "refine");
                        LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL,
                               "changing config from 'true' to 'false' is prohibited while the target "
                               "has still a children with explicit config 'true'.");
                        goto fail;
                    }
                }
                /* change config */
                iter->flags &= ~LYS_CONFIG_MASK;
                iter->flags |= (rfn->flags & LYS_CONFIG_MASK);

                /* select next iter - modified LY_TREE_DFS_END */
                if (iter->nodetype & (LYS_LEAF | LYS_LEAFLIST | LYS_ANYDATA)) {
                    next = NULL;
                } else {
                    next = iter->child;
                }
nextsibling:
                if (!next) {
                    /* try siblings */
                    next = iter->next;
                }
                while (!next) {
                    /* parent is already processed, go to its sibling */
                    iter = lys_parent(iter);

                    /* no siblings, go back through parents */
                    if (iter == node) {
                        /* we are done, no next element to process */
                        break;
                    }
                    next = iter->next;
                }
            }
        }

        /* default value */
        if (rfn->dflt_size) {
            if (node->nodetype == LYS_CHOICE) {
                /* choice */
                ((struct lys_node_choice *)node)->dflt = resolve_choice_dflt((struct lys_node_choice *)node,
                                                                             rfn->dflt[0]);
                if (!((struct lys_node_choice *)node)->dflt) {
                    LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, uses, rfn->dflt[0], "default");
                    goto fail;
                }
                if (lyp_check_mandatory_choice(node)) {
                    goto fail;
                }
            }
        }

        /* min/max-elements on list or leaf-list */
        if (node->nodetype == LYS_LIST && ((struct lys_node_list *)node)->max) {
            if (((struct lys_node_list *)node)->min > ((struct lys_node_list *)node)->max) {
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_LYS, uses, "Invalid value \"%d\" of \"%s\".", rfn->mod.list.min, "min-elements");
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "\"min-elements\" is bigger than \"max-elements\".");
                goto fail;
            }
        } else if (node->nodetype == LYS_LEAFLIST && ((struct lys_node_leaflist *)node)->max) {
            if (((struct lys_node_leaflist *)node)->min > ((struct lys_node_leaflist *)node)->max) {
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_LYS, uses, "Invalid value \"%d\" of \"%s\".", rfn->mod.list.min, "min-elements");
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "\"min-elements\" is bigger than \"max-elements\".");
                goto fail;
            }
        }

        /* additional checks */
        /* default value with mandatory/min-elements */
        if (node->nodetype == LYS_LEAFLIST) {
            llist = (struct lys_node_leaflist *)node;
            if (llist->dflt_size && llist->min) {
                LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, uses, rfn->dflt_size ? "default" : "min-elements", "refine");
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL,
                       "The \"min-elements\" statement with non-zero value is forbidden on leaf-lists with the \"default\" statement.");
                goto fail;
            }
        } else if (node->nodetype == LYS_LEAF) {
            leaf = (struct lys_node_leaf *)node;
            if (leaf->dflt && (leaf->flags & LYS_MAND_TRUE)) {
                LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, uses, rfn->dflt_size ? "default" : "mandatory", "refine");
                LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL,
                       "The \"mandatory\" statement is forbidden on leafs with the \"default\" statement.");
                goto fail;
            }
        }

        /* check for mandatory node in default case, first find the closest parent choice to the changed node */
        if ((rfn->flags & LYS_MAND_TRUE) || rfn->mod.list.min) {
            for (parent = node->parent;
                 parent && !(parent->nodetype & (LYS_CHOICE | LYS_GROUPING | LYS_ACTION | LYS_USES));
                 parent = parent->parent) {
                if (parent->nodetype == LYS_CONTAINER && ((struct lys_node_container *)parent)->presence) {
                    /* stop also on presence containers */
                    break;
                }
            }
            /* and if it is a choice with the default case, check it for presence of a mandatory node in it */
            if (parent && parent->nodetype == LYS_CHOICE && ((struct lys_node_choice *)parent)->dflt) {
                if (lyp_check_mandatory_choice(parent)) {
                    goto fail;
                }
            }
        }
    }
    free(refine_nodes);

    /* check list config after all the refines were applied */
    LY_TREE_DFS_BEGIN((struct lys_node *)uses, next, iter) {
        if ((iter->nodetype == LYS_LIST) && (iter->flags & LYS_CONFIG_W)
                && !((struct lys_node_list *)iter)->keys_size) {
            LOGVAL(ctx, LYE_MISSCHILDSTMT, LY_VLOG_LYS, iter, "key", "list");
            goto fail;
        }
        LY_TREE_DFS_END((struct lys_node *)uses, next, iter);
    }

    return EXIT_SUCCESS;

fail:
    LY_TREE_FOR_SAFE(uses->child, next, iter) {
        lys_node_free(iter, NULL, 0);
    }
    free(refine_nodes);
    return -1;
}