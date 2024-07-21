rsvg_filter_render (RsvgFilter * self, GdkPixbuf * source,
                    RsvgDrawingCtx * context, RsvgBbox * bounds, char *channelmap)
{
    RsvgFilterContext *ctx;
    RsvgFilterPrimitive *current;
    guint i;
    GdkPixbuf *out;


    ctx = g_new (RsvgFilterContext, 1);
    ctx->filter = self;
    ctx->source = source;
    ctx->bg = NULL;
    ctx->results = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, rsvg_filter_free_pair);
    ctx->ctx = context;

    g_object_ref (source);

    rsvg_filter_fix_coordinate_system (ctx, rsvg_current_state (context), *bounds);

    ctx->lastresult.result = source;
    ctx->lastresult.Rused = 1;
    ctx->lastresult.Gused = 1;
    ctx->lastresult.Bused = 1;
    ctx->lastresult.Aused = 1;
    ctx->lastresult.bounds = rsvg_filter_primitive_get_bounds (NULL, ctx);

    for (i = 0; i < 4; i++)
        ctx->channelmap[i] = channelmap[i] - '0';

    for (i = 0; i < self->super.children->len; i++) {
        current = g_ptr_array_index (self->super.children, i);
        if (!strncmp (current->super.type->str, "fe", 2))
            rsvg_filter_primitive_render (current, ctx);
    }

    out = ctx->lastresult.result;

    g_hash_table_destroy (ctx->results);

    rsvg_filter_context_free (ctx);

    return out;
}