lys_node_free(struct lys_node *node, void (*private_destructor)(const struct lys_node *node, void *priv), int shallow)
{
    struct ly_ctx *ctx;
    struct lys_node *sub, *next;

    if (!node) {
        return;
    }

    assert(node->module);
    assert(node->module->ctx);

    ctx = node->module->ctx;

    /* remove private object */
    if (node->priv && private_destructor) {
        private_destructor(node, node->priv);
    }

    /* common part */
    lydict_remove(ctx, node->name);
    if (!(node->nodetype & (LYS_INPUT | LYS_OUTPUT))) {
        lys_iffeature_free(ctx, node->iffeature, node->iffeature_size, shallow, private_destructor);
        lydict_remove(ctx, node->dsc);
        lydict_remove(ctx, node->ref);
    }

    if (!shallow && !(node->nodetype & (LYS_LEAF | LYS_LEAFLIST))) {
        LY_TREE_FOR_SAFE(node->child, next, sub) {
            lys_node_free(sub, private_destructor, 0);
        }
    }

    lys_extension_instances_free(ctx, node->ext, node->ext_size, private_destructor);

    /* specific part */
    switch (node->nodetype) {
    case LYS_CONTAINER:
        lys_container_free(ctx, (struct lys_node_container *)node, private_destructor);
        break;
    case LYS_CHOICE:
        lys_when_free(ctx, ((struct lys_node_choice *)node)->when, private_destructor);
        break;
    case LYS_LEAF:
        lys_leaf_free(ctx, (struct lys_node_leaf *)node, private_destructor);
        break;
    case LYS_LEAFLIST:
        lys_leaflist_free(ctx, (struct lys_node_leaflist *)node, private_destructor);
        break;
    case LYS_LIST:
        lys_list_free(ctx, (struct lys_node_list *)node, private_destructor);
        break;
    case LYS_ANYXML:
    case LYS_ANYDATA:
        lys_anydata_free(ctx, (struct lys_node_anydata *)node, private_destructor);
        break;
    case LYS_USES:
        lys_uses_free(ctx, (struct lys_node_uses *)node, private_destructor);
        break;
    case LYS_CASE:
        lys_when_free(ctx, ((struct lys_node_case *)node)->when, private_destructor);
        break;
    case LYS_AUGMENT:
        /* do nothing */
        break;
    case LYS_GROUPING:
        lys_grp_free(ctx, (struct lys_node_grp *)node, private_destructor);
        break;
    case LYS_RPC:
    case LYS_ACTION:
        lys_rpc_action_free(ctx, (struct lys_node_rpc_action *)node, private_destructor);
        break;
    case LYS_NOTIF:
        lys_notif_free(ctx, (struct lys_node_notif *)node, private_destructor);
        break;
    case LYS_INPUT:
    case LYS_OUTPUT:
        lys_inout_free(ctx, (struct lys_node_inout *)node, private_destructor);
        break;
    case LYS_EXT:
    case LYS_UNKNOWN:
        LOGINT(ctx);
        break;
    }

    /* again common part */
    lys_node_unlink(node);
    free(node);
}