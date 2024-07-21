module_free_common(struct lys_module *module, void (*private_destructor)(const struct lys_node *node, void *priv))
{
    struct ly_ctx *ctx;
    struct lys_node *next, *iter;
    unsigned int i;

    assert(module->ctx);
    ctx = module->ctx;

    /* just free the import array, imported modules will stay in the context */
    for (i = 0; i < module->imp_size; i++) {
        lydict_remove(ctx, module->imp[i].prefix);
        lydict_remove(ctx, module->imp[i].dsc);
        lydict_remove(ctx, module->imp[i].ref);
        lys_extension_instances_free(ctx, module->imp[i].ext, module->imp[i].ext_size, private_destructor);
    }
    free(module->imp);

    /* submodules don't have data tree, the data nodes
     * are placed in the main module altogether */
    if (!module->type) {
        LY_TREE_FOR_SAFE(module->data, next, iter) {
            lys_node_free(iter, private_destructor, 0);
        }
    }

    lydict_remove(ctx, module->dsc);
    lydict_remove(ctx, module->ref);
    lydict_remove(ctx, module->org);
    lydict_remove(ctx, module->contact);
    lydict_remove(ctx, module->filepath);

    /* revisions */
    for (i = 0; i < module->rev_size; i++) {
        lys_extension_instances_free(ctx, module->rev[i].ext, module->rev[i].ext_size, private_destructor);
        lydict_remove(ctx, module->rev[i].dsc);
        lydict_remove(ctx, module->rev[i].ref);
    }
    free(module->rev);

    /* identities */
    for (i = 0; i < module->ident_size; i++) {
        lys_ident_free(ctx, &module->ident[i], private_destructor);
    }
    module->ident_size = 0;
    free(module->ident);

    /* typedefs */
    for (i = 0; i < module->tpdf_size; i++) {
        lys_tpdf_free(ctx, &module->tpdf[i], private_destructor);
    }
    free(module->tpdf);

    /* extension instances */
    lys_extension_instances_free(ctx, module->ext, module->ext_size, private_destructor);

    /* augment */
    for (i = 0; i < module->augment_size; i++) {
        lys_augment_free(ctx, &module->augment[i], private_destructor);
    }
    free(module->augment);

    /* features */
    for (i = 0; i < module->features_size; i++) {
        lys_feature_free(ctx, &module->features[i], private_destructor);
    }
    free(module->features);

    /* deviations */
    for (i = 0; i < module->deviation_size; i++) {
        lys_deviation_free(module, &module->deviation[i], private_destructor);
    }
    free(module->deviation);

    /* extensions */
    for (i = 0; i < module->extensions_size; i++) {
        lys_extension_free(ctx, &module->extensions[i], private_destructor);
    }
    free(module->extensions);

    lydict_remove(ctx, module->name);
    lydict_remove(ctx, module->prefix);
}