rsvg_filter_primitive_image_render_in (RsvgFilterPrimitive * self, RsvgFilterContext * context)
{
    RsvgDrawingCtx *ctx;
    RsvgFilterPrimitiveImage *upself;
    RsvgNode *drawable;

    ctx = context->ctx;

    upself = (RsvgFilterPrimitiveImage *) self;

    if (!upself->href)
        return NULL;

    drawable = rsvg_defs_lookup (ctx->defs, upself->href->str);
    if (!drawable)
        return NULL;

    rsvg_current_state (ctx)->affine = context->paffine;

    return rsvg_get_surface_of_node (ctx, drawable, context->width, context->height);
}