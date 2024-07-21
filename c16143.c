rsvg_filter_primitive_merge_render (RsvgFilterPrimitive * self, RsvgFilterContext * ctx)
{
    guint i;
    RsvgIRect boundarys;

    RsvgFilterPrimitiveMerge *upself;

    GdkPixbuf *output;
    GdkPixbuf *in;

    upself = (RsvgFilterPrimitiveMerge *) self;
    boundarys = rsvg_filter_primitive_get_bounds (self, ctx);

    output = _rsvg_pixbuf_new_cleared (GDK_COLORSPACE_RGB, 1, 8, ctx->width, ctx->height);

    for (i = 0; i < upself->super.super.children->len; i++) {
        RsvgFilterPrimitive *mn;
        mn = g_ptr_array_index (upself->super.super.children, i);
        if (strcmp (mn->super.type->str, "feMergeNode"))
            continue;
        in = rsvg_filter_get_in (mn->in, ctx);
        rsvg_alpha_blt (in, boundarys.x0, boundarys.y0, boundarys.x1 - boundarys.x0,
                        boundarys.y1 - boundarys.y0, output, boundarys.x0, boundarys.y0);
        g_object_unref (in);
    }

    rsvg_filter_store_result (self->result, output, ctx);

    g_object_unref (output);
}