lys_augment_free(struct ly_ctx *ctx, struct lys_node_augment *aug,
                 void (*private_destructor)(const struct lys_node *node, void *priv))
{
    struct lys_node *next, *sub;

    /* children from a resolved augment are freed under the target node */
    if (!aug->target || (aug->flags & LYS_NOTAPPLIED)) {
        LY_TREE_FOR_SAFE(aug->child, next, sub) {
            lys_node_free(sub, private_destructor, 0);
        }
    }

    lydict_remove(ctx, aug->target_name);
    lydict_remove(ctx, aug->dsc);
    lydict_remove(ctx, aug->ref);

    lys_iffeature_free(ctx, aug->iffeature, aug->iffeature_size, 0, private_destructor);
    lys_extension_instances_free(ctx, aug->ext, aug->ext_size, private_destructor);

    lys_when_free(ctx, aug->when, private_destructor);
}