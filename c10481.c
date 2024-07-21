lys_node_addchild(struct lys_node *parent, struct lys_module *module, struct lys_node *child, int options)
{
    struct ly_ctx *ctx = child->module->ctx;
    struct lys_node *iter, **pchild, *log_parent;
    struct lys_node_inout *in, *out;
    struct lys_node_case *c;
    struct lys_node_augment *aug;
    int type, shortcase = 0;
    void *p;
    struct lyext_substmt *info = NULL;

    assert(child);

    if (parent) {
        type = parent->nodetype;
        module = parent->module;
        log_parent = parent;

        if (type == LYS_USES) {
            /* we are adding children to uses -> we must be copying grouping contents into it, so properly check the parent */
            while (log_parent && (log_parent->nodetype == LYS_USES)) {
                if (log_parent->nodetype == LYS_AUGMENT) {
                    aug = (struct lys_node_augment *)log_parent;
                    if (!aug->target) {
                        /* unresolved augment, just pass the node type check */
                        goto skip_nodetype_check;
                    }
                    log_parent = aug->target;
                } else {
                    log_parent = log_parent->parent;
                }
            }
            if (log_parent) {
                type = log_parent->nodetype;
            } else {
                type = 0;
            }
        }
    } else {
        assert(module);
        assert(!(child->nodetype & (LYS_INPUT | LYS_OUTPUT)));
        type = 0;
        log_parent = NULL;
    }

    /* checks */
    switch (type) {
    case LYS_CONTAINER:
    case LYS_LIST:
    case LYS_GROUPING:
    case LYS_USES:
        if (!(child->nodetype &
                (LYS_ANYDATA | LYS_CHOICE | LYS_CONTAINER | LYS_GROUPING | LYS_LEAF |
                 LYS_LEAFLIST | LYS_LIST | LYS_USES | LYS_ACTION | LYS_NOTIF))) {
            LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype), strnodetype(log_parent->nodetype));
            return EXIT_FAILURE;
        }
        break;
    case LYS_INPUT:
    case LYS_OUTPUT:
    case LYS_NOTIF:
        if (!(child->nodetype &
                (LYS_ANYDATA | LYS_CHOICE | LYS_CONTAINER | LYS_GROUPING | LYS_LEAF |
                 LYS_LEAFLIST | LYS_LIST | LYS_USES))) {
            LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype), strnodetype(log_parent->nodetype));
            return EXIT_FAILURE;
        }
        break;
    case LYS_CHOICE:
        if (!(child->nodetype &
                (LYS_ANYDATA | LYS_CASE | LYS_CONTAINER | LYS_LEAF | LYS_LEAFLIST | LYS_LIST | LYS_CHOICE))) {
            LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype), "choice");
            return EXIT_FAILURE;
        }
        if (child->nodetype != LYS_CASE) {
            shortcase = 1;
        }
        break;
    case LYS_CASE:
        if (!(child->nodetype &
                (LYS_ANYDATA | LYS_CHOICE | LYS_CONTAINER | LYS_LEAF | LYS_LEAFLIST | LYS_LIST | LYS_USES))) {
            LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype), "case");
            return EXIT_FAILURE;
        }
        break;
    case LYS_RPC:
    case LYS_ACTION:
        if (!(child->nodetype & (LYS_INPUT | LYS_OUTPUT | LYS_GROUPING))) {
            LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype), "rpc");
            return EXIT_FAILURE;
        }
        break;
    case LYS_LEAF:
    case LYS_LEAFLIST:
    case LYS_ANYXML:
    case LYS_ANYDATA:
        LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype), strnodetype(log_parent->nodetype));
        LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "The \"%s\" statement cannot have any data substatement.",
               strnodetype(log_parent->nodetype));
        return EXIT_FAILURE;
    case LYS_AUGMENT:
        if (!(child->nodetype &
                (LYS_ANYDATA | LYS_CASE | LYS_CHOICE | LYS_CONTAINER | LYS_LEAF
                | LYS_LEAFLIST | LYS_LIST | LYS_USES | LYS_ACTION | LYS_NOTIF))) {
            LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype), strnodetype(log_parent->nodetype));
            return EXIT_FAILURE;
        }
        break;
    case LYS_UNKNOWN:
        /* top level */
        if (!(child->nodetype &
                (LYS_ANYDATA | LYS_CHOICE | LYS_CONTAINER | LYS_LEAF | LYS_GROUPING
                | LYS_LEAFLIST | LYS_LIST | LYS_USES | LYS_RPC | LYS_NOTIF | LYS_AUGMENT))) {
            LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype), "(sub)module");
            return EXIT_FAILURE;
        }
        break;
    case LYS_EXT:
        /* plugin-defined */
        p = lys_ext_complex_get_substmt(lys_snode2stmt(child->nodetype), (struct lys_ext_instance_complex*)log_parent, &info);
        if (!p) {
            LOGVAL(ctx, LYE_INCHILDSTMT, LY_VLOG_LYS, log_parent, strnodetype(child->nodetype),
                   ((struct lys_ext_instance_complex*)log_parent)->def->name);
            return EXIT_FAILURE;
        }
        /* TODO check cardinality */
        break;
    }

skip_nodetype_check:
    /* check identifier uniqueness */
    if (!(module->ctx->models.flags & LY_CTX_TRUSTED) && lys_check_id(child, parent, module)) {
        return EXIT_FAILURE;
    }

    if (child->parent) {
        lys_node_unlink(child);
    }

    if ((child->nodetype & (LYS_INPUT | LYS_OUTPUT)) && parent->nodetype != LYS_EXT) {
        /* find the implicit input/output node */
        LY_TREE_FOR(parent->child, iter) {
            if (iter->nodetype == child->nodetype) {
                break;
            }
        }
        assert(iter);

        /* switch the old implicit node (iter) with the new one (child) */
        if (parent->child == iter) {
            /* first child */
            parent->child = child;
        } else {
            iter->prev->next = child;
        }
        child->prev = iter->prev;
        child->next = iter->next;
        if (iter->next) {
            iter->next->prev = child;
        } else {
            /* last child */
            parent->child->prev = child;
        }
        child->parent = parent;

        /* isolate the node and free it */
        iter->next = NULL;
        iter->prev = iter;
        iter->parent = NULL;
        lys_node_free(iter, NULL, 0);
    } else {
        if (shortcase) {
            /* create the implicit case to allow it to serve as a target of the augments,
             * it won't be printed, but it will be present in the tree */
            c = calloc(1, sizeof *c);
            LY_CHECK_ERR_RETURN(!c, LOGMEM(ctx), EXIT_FAILURE);
            c->name = lydict_insert(module->ctx, child->name, 0);
            c->flags = LYS_IMPLICIT;
            if (!(options & (LYS_PARSE_OPT_CFG_IGNORE | LYS_PARSE_OPT_CFG_NOINHERIT))) {
                /* get config flag from parent */
                c->flags |= parent->flags & LYS_CONFIG_MASK;
            }
            c->module = module;
            c->nodetype = LYS_CASE;
            c->prev = (struct lys_node*)c;
            lys_node_addchild(parent, module, (struct lys_node*)c, options);
            parent = (struct lys_node*)c;
        }
        /* connect the child correctly */
        if (!parent) {
            if (module->data) {
                module->data->prev->next = child;
                child->prev = module->data->prev;
                module->data->prev = child;
            } else {
                module->data = child;
            }
        } else {
            pchild = lys_child(parent, child->nodetype);
            assert(pchild);

            child->parent = parent;
            if (!(*pchild)) {
                /* the only/first child of the parent */
                *pchild = child;
                iter = child;
            } else {
                /* add a new child at the end of parent's child list */
                iter = (*pchild)->prev;
                iter->next = child;
                child->prev = iter;
            }
            while (iter->next) {
                iter = iter->next;
                iter->parent = parent;
            }
            (*pchild)->prev = iter;
        }
    }

    /* check config value (but ignore them in groupings and augments) */
    for (iter = parent; iter && !(iter->nodetype & (LYS_GROUPING | LYS_AUGMENT | LYS_EXT)); iter = iter->parent);
    if (parent && !iter) {
        for (iter = child; iter && !(iter->nodetype & (LYS_NOTIF | LYS_INPUT | LYS_OUTPUT | LYS_RPC)); iter = iter->parent);
        if (!iter && (parent->flags & LYS_CONFIG_R) && (child->flags & LYS_CONFIG_W)) {
            LOGVAL(ctx, LYE_INARG, LY_VLOG_LYS, child, "true", "config");
            LOGVAL(ctx, LYE_SPEC, LY_VLOG_PREV, NULL, "State nodes cannot have configuration nodes as children.");
            return EXIT_FAILURE;
        }
    }

    /* propagate information about status data presence */
    if ((child->nodetype & (LYS_CONTAINER | LYS_CHOICE | LYS_LEAF | LYS_LEAFLIST | LYS_LIST | LYS_ANYDATA)) &&
            (child->flags & LYS_INCL_STATUS)) {
        for(iter = parent; iter; iter = lys_parent(iter)) {
            /* store it only into container or list - the only data inner nodes */
            if (iter->nodetype & (LYS_CONTAINER | LYS_LIST)) {
                if (iter->flags & LYS_INCL_STATUS) {
                    /* done, someone else set it already from here */
                    break;
                }
                /* set flag about including status data */
                iter->flags |= LYS_INCL_STATUS;
            }
        }
    }

    /* create implicit input/output nodes to have available them as possible target for augment */
    if (child->nodetype & (LYS_RPC | LYS_ACTION)) {
        if (child->nodetype == LYS_ACTION) {
            for (iter = child->parent; iter; iter = lys_parent(iter)) {
                if ((iter->nodetype & (LYS_RPC | LYS_ACTION | LYS_NOTIF))
                        || ((iter->nodetype == LYS_LIST) && !((struct lys_node_list *)iter)->keys)) {
                    LOGVAL(module->ctx, LYE_INPAR, LY_VLOG_LYS, iter, strnodetype(iter->nodetype), "action");
                    return EXIT_FAILURE;
                }
            }
        }

        if (!child->child) {
            in = calloc(1, sizeof *in);
            out = calloc(1, sizeof *out);
            if (!in || !out) {
                LOGMEM(ctx);
                free(in);
                free(out);
                return EXIT_FAILURE;
            }
            in->nodetype = LYS_INPUT;
            in->name = lydict_insert(child->module->ctx, "input", 5);
            out->nodetype = LYS_OUTPUT;
            out->name = lydict_insert(child->module->ctx, "output", 6);
            in->module = out->module = child->module;
            in->parent = out->parent = child;
            in->flags = out->flags = LYS_IMPLICIT;
            in->next = (struct lys_node *)out;
            in->prev = (struct lys_node *)out;
            out->prev = (struct lys_node *)in;
            child->child = (struct lys_node *)in;
        }
    }
    return EXIT_SUCCESS;
}