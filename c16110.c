rsvg_pop_def_group (RsvgHandle * ctx)
{
    if (ctx->priv->currentnode != NULL)
        ctx->priv->currentnode = ctx->priv->currentnode->parent;
}