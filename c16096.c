rsvg_end_element (void *data, const xmlChar * name)
{
    RsvgHandle *ctx = (RsvgHandle *) data;

    if (ctx->priv->handler_nest > 0 && ctx->priv->handler != NULL) {
        if (ctx->priv->handler->end_element != NULL)
            ctx->priv->handler->end_element (ctx->priv->handler, (const char *) name);
        ctx->priv->handler_nest--;
    } else {
        const char *tempname;
        for (tempname = (const char *) name; *tempname != '\0'; tempname++)
            if (*tempname == ':')
                name = (const xmlChar *) (tempname + 1);

        if (ctx->priv->handler != NULL) {
            ctx->priv->handler->free (ctx->priv->handler);
            ctx->priv->handler = NULL;
        }

        if (ctx->priv->currentnode
            && !strcmp ((const char *) name, ctx->priv->currentnode->type->str))
            rsvg_pop_def_group (ctx);

        if (ctx->priv->treebase && !strcmp ((const char *)name, "svg"))
            _rsvg_node_svg_apply_atts ((RsvgNodeSvg *)ctx->priv->treebase, ctx);
    }
}