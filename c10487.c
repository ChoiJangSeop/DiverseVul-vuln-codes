lys_extension_instances_free(struct ly_ctx *ctx, struct lys_ext_instance **e, unsigned int size,
                             void (*private_destructor)(const struct lys_node *node, void *priv))
{
    unsigned int i, j, k;
    struct lyext_substmt *substmt;
    void **pp, **start;
    struct lys_node *siter, *snext;

#define EXTCOMPLEX_FREE_STRUCT(STMT, TYPE, FUNC, FREE, ARGS...)                               \
    pp = lys_ext_complex_get_substmt(STMT, (struct lys_ext_instance_complex *)e[i], NULL);    \
    if (!pp || !(*pp)) { break; }                                                             \
    if (substmt[j].cardinality >= LY_STMT_CARD_SOME) { /* process array */                    \
        for (start = pp = *pp; *pp; pp++) {                                                   \
            FUNC(ctx, (TYPE *)(*pp), ##ARGS, private_destructor);                             \
            if (FREE) { free(*pp); }                                                          \
        }                                                                                     \
        free(start);                                                                          \
    } else { /* single item */                                                                \
        FUNC(ctx, (TYPE *)(*pp), ##ARGS, private_destructor);                                 \
        if (FREE) { free(*pp); }                                                              \
    }

    if (!size || !e) {
        return;
    }

    for (i = 0; i < size; i++) {
        if (!e[i]) {
            continue;
        }

        if (e[i]->flags & (LYEXT_OPT_INHERIT)) {
            /* no free, this is just a shadow copy of the original extension instance */
        } else {
            if (e[i]->flags & (LYEXT_OPT_YANG)) {
                free(e[i]->def);     /* remove name of instance extension */
                e[i]->def = NULL;
                yang_free_ext_data((struct yang_ext_substmt *)e[i]->parent); /* remove backup part of yang file */
            }
            /* remove private object */
            if (e[i]->priv && private_destructor) {
                private_destructor((struct lys_node*)e[i], e[i]->priv);
            }
            lys_extension_instances_free(ctx, e[i]->ext, e[i]->ext_size, private_destructor);
            lydict_remove(ctx, e[i]->arg_value);
        }

        if (e[i]->def && e[i]->def->plugin && e[i]->def->plugin->type == LYEXT_COMPLEX
                && ((e[i]->flags & LYEXT_OPT_CONTENT) == 0)) {
            substmt = ((struct lys_ext_instance_complex *)e[i])->substmt;
            for (j = 0; substmt[j].stmt; j++) {
                switch(substmt[j].stmt) {
                case LY_STMT_DESCRIPTION:
                case LY_STMT_REFERENCE:
                case LY_STMT_UNITS:
                case LY_STMT_ARGUMENT:
                case LY_STMT_DEFAULT:
                case LY_STMT_ERRTAG:
                case LY_STMT_ERRMSG:
                case LY_STMT_PREFIX:
                case LY_STMT_NAMESPACE:
                case LY_STMT_PRESENCE:
                case LY_STMT_REVISIONDATE:
                case LY_STMT_KEY:
                case LY_STMT_BASE:
                case LY_STMT_BELONGSTO:
                case LY_STMT_CONTACT:
                case LY_STMT_ORGANIZATION:
                case LY_STMT_PATH:
                    lys_extcomplex_free_str(ctx, (struct lys_ext_instance_complex *)e[i], substmt[j].stmt);
                    break;
                case LY_STMT_TYPE:
                    EXTCOMPLEX_FREE_STRUCT(LY_STMT_TYPE, struct lys_type, lys_type_free, 1);
                    break;
                case LY_STMT_TYPEDEF:
                    EXTCOMPLEX_FREE_STRUCT(LY_STMT_TYPEDEF, struct lys_tpdf, lys_tpdf_free, 1);
                    break;
                case LY_STMT_IFFEATURE:
                    EXTCOMPLEX_FREE_STRUCT(LY_STMT_IFFEATURE, struct lys_iffeature, lys_iffeature_free, 0, 1, 0);
                    break;
                case LY_STMT_MAX:
                case LY_STMT_MIN:
                case LY_STMT_POSITION:
                case LY_STMT_VALUE:
                    pp = (void**)&((struct lys_ext_instance_complex *)e[i])->content[substmt[j].offset];
                    if (substmt[j].cardinality >= LY_STMT_CARD_SOME && *pp) {
                        for(k = 0; ((uint32_t**)(*pp))[k]; k++) {
                            free(((uint32_t**)(*pp))[k]);
                        }
                    }
                    free(*pp);
                    break;
                case LY_STMT_DIGITS:
                    if (substmt[j].cardinality >= LY_STMT_CARD_SOME) {
                        /* free the array */
                        pp = (void**)&((struct lys_ext_instance_complex *)e[i])->content[substmt[j].offset];
                        free(*pp);
                    }
                    break;
                case LY_STMT_MODULE:
                    /* modules are part of the context, so they will be freed there */
                    if (substmt[j].cardinality >= LY_STMT_CARD_SOME) {
                        /* free the array */
                        pp = (void**)&((struct lys_ext_instance_complex *)e[i])->content[substmt[j].offset];
                        free(*pp);
                    }
                    break;
                case LY_STMT_ACTION:
                case LY_STMT_ANYDATA:
                case LY_STMT_ANYXML:
                case LY_STMT_CASE:
                case LY_STMT_CHOICE:
                case LY_STMT_CONTAINER:
                case LY_STMT_GROUPING:
                case LY_STMT_INPUT:
                case LY_STMT_LEAF:
                case LY_STMT_LEAFLIST:
                case LY_STMT_LIST:
                case LY_STMT_NOTIFICATION:
                case LY_STMT_OUTPUT:
                case LY_STMT_RPC:
                case LY_STMT_USES:
                    pp = (void**)&((struct lys_ext_instance_complex *)e[i])->content[substmt[j].offset];
                    LY_TREE_FOR_SAFE((struct lys_node *)(*pp), snext, siter) {
                        lys_node_free(siter, NULL, 0);
                    }
                    *pp = NULL;
                    break;
                case LY_STMT_UNIQUE:
                    pp = lys_ext_complex_get_substmt(LY_STMT_UNIQUE, (struct lys_ext_instance_complex *)e[i], NULL);
                    if (!pp || !(*pp)) {
                        break;
                    }
                    if (substmt[j].cardinality >= LY_STMT_CARD_SOME) { /* process array */
                        for (start = pp = *pp; *pp; pp++) {
                            for (k = 0; k < (*(struct lys_unique**)pp)->expr_size; k++) {
                                lydict_remove(ctx, (*(struct lys_unique**)pp)->expr[k]);
                            }
                            free((*(struct lys_unique**)pp)->expr);
                            free(*pp);
                        }
                        free(start);
                    } else { /* single item */
                        for (k = 0; k < (*(struct lys_unique**)pp)->expr_size; k++) {
                            lydict_remove(ctx, (*(struct lys_unique**)pp)->expr[k]);
                        }
                        free((*(struct lys_unique**)pp)->expr);
                        free(*pp);
                    }
                    break;
                case LY_STMT_LENGTH:
                case LY_STMT_MUST:
                case LY_STMT_PATTERN:
                case LY_STMT_RANGE:
                    EXTCOMPLEX_FREE_STRUCT(substmt[j].stmt, struct lys_restr, lys_restr_free, 1);
                    break;
                case LY_STMT_WHEN:
                    EXTCOMPLEX_FREE_STRUCT(LY_STMT_WHEN, struct lys_when, lys_when_free, 0);
                    break;
                case LY_STMT_REVISION:
                    pp = lys_ext_complex_get_substmt(LY_STMT_REVISION, (struct lys_ext_instance_complex *)e[i], NULL);
                    if (!pp || !(*pp)) {
                        break;
                    }
                    if (substmt[j].cardinality >= LY_STMT_CARD_SOME) { /* process array */
                        for (start = pp = *pp; *pp; pp++) {
                            lydict_remove(ctx, (*(struct lys_revision**)pp)->dsc);
                            lydict_remove(ctx, (*(struct lys_revision**)pp)->ref);
                            lys_extension_instances_free(ctx, (*(struct lys_revision**)pp)->ext,
                                                         (*(struct lys_revision**)pp)->ext_size, private_destructor);
                            free(*pp);
                        }
                        free(start);
                    } else { /* single item */
                        lydict_remove(ctx, (*(struct lys_revision**)pp)->dsc);
                        lydict_remove(ctx, (*(struct lys_revision**)pp)->ref);
                        lys_extension_instances_free(ctx, (*(struct lys_revision**)pp)->ext,
                                                     (*(struct lys_revision**)pp)->ext_size, private_destructor);
                        free(*pp);
                    }
                    break;
                default:
                    /* nothing to free */
                    break;
                }
            }
        }

        free(e[i]);
    }
    free(e);

#undef EXTCOMPLEX_FREE_STRUCT
}