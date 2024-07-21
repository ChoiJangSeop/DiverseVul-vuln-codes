lys_deviation_free(struct lys_module *module, struct lys_deviation *dev,
                   void (*private_destructor)(const struct lys_node *node, void *priv))
{
    int i, j, k;
    struct ly_ctx *ctx;
    struct lys_node *next, *elem;

    ctx = module->ctx;

    lydict_remove(ctx, dev->target_name);
    lydict_remove(ctx, dev->dsc);
    lydict_remove(ctx, dev->ref);
    lys_extension_instances_free(ctx, dev->ext, dev->ext_size, private_destructor);

    if (!dev->deviate) {
        return;
    }

    /* it could not be applied because it failed to be applied */
    if (dev->orig_node) {
        /* the module was freed, but we only need the context from orig_node, use ours */
        if (dev->deviate[0].mod == LY_DEVIATE_NO) {
            /* it's actually a node subtree, we need to update modules on all the nodes :-/ */
            LY_TREE_DFS_BEGIN(dev->orig_node, next, elem) {
                elem->module = module;

                LY_TREE_DFS_END(dev->orig_node, next, elem);
            }
            lys_node_free(dev->orig_node, NULL, 0);
        } else {
            /* it's just a shallow copy, freeing one node */
            dev->orig_node->module = module;
            lys_node_free(dev->orig_node, NULL, 1);
        }
    }

    for (i = 0; i < dev->deviate_size; i++) {
        lys_extension_instances_free(ctx, dev->deviate[i].ext, dev->deviate[i].ext_size, private_destructor);

        for (j = 0; j < dev->deviate[i].dflt_size; j++) {
            lydict_remove(ctx, dev->deviate[i].dflt[j]);
        }
        free(dev->deviate[i].dflt);

        lydict_remove(ctx, dev->deviate[i].units);

        if (dev->deviate[i].mod == LY_DEVIATE_DEL) {
            for (j = 0; j < dev->deviate[i].must_size; j++) {
                lys_restr_free(ctx, &dev->deviate[i].must[j], private_destructor);
            }
            free(dev->deviate[i].must);

            for (j = 0; j < dev->deviate[i].unique_size; j++) {
                for (k = 0; k < dev->deviate[i].unique[j].expr_size; k++) {
                    lydict_remove(ctx, dev->deviate[i].unique[j].expr[k]);
                }
                free(dev->deviate[i].unique[j].expr);
            }
            free(dev->deviate[i].unique);
        }
    }
    free(dev->deviate);
}