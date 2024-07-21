rsvg_filter_primitive_component_transfer_render (RsvgFilterPrimitive *
                                                 self, RsvgFilterContext * ctx)
{
    gint x, y, c;
    guint i;
    gint temp;
    gint rowstride, height, width;
    RsvgIRect boundarys;
    RsvgNodeComponentTransferFunc *channels[4];
    ComponentTransferFunc functions[4];
    guchar *inpix, outpix[4];
    gint achan = ctx->channelmap[3];
    guchar *in_pixels;
    guchar *output_pixels;

    RsvgFilterPrimitiveComponentTransfer *upself;

    GdkPixbuf *output;
    GdkPixbuf *in;
    upself = (RsvgFilterPrimitiveComponentTransfer *) self;
    boundarys = rsvg_filter_primitive_get_bounds (self, ctx);

    for (c = 0; c < 4; c++) {
        char channel = "RGBA"[c];
        for (i = 0; i < self->super.children->len; i++) {
            RsvgNodeComponentTransferFunc *temp;
            temp = (RsvgNodeComponentTransferFunc *)
                g_ptr_array_index (self->super.children, i);
            if (!strncmp (temp->super.type->str, "feFunc", 6))
                if (temp->super.type->str[6] == channel) {
                    functions[ctx->channelmap[c]] = temp->function;
                    channels[ctx->channelmap[c]] = temp;
                    break;
                }
        }
        if (i == self->super.children->len)
            functions[ctx->channelmap[c]] = identity_component_transfer_func;

    }

    in = rsvg_filter_get_in (self->in, ctx);
    in_pixels = gdk_pixbuf_get_pixels (in);

    height = gdk_pixbuf_get_height (in);
    width = gdk_pixbuf_get_width (in);

    rowstride = gdk_pixbuf_get_rowstride (in);

    output = _rsvg_pixbuf_new_cleared (GDK_COLORSPACE_RGB, 1, 8, width, height);

    output_pixels = gdk_pixbuf_get_pixels (output);

    for (y = boundarys.y0; y < boundarys.y1; y++)
        for (x = boundarys.x0; x < boundarys.x1; x++) {
            inpix = in_pixels + (y * rowstride + x * 4);
            for (c = 0; c < 4; c++) {
                int inval;
                if (c != achan) {
                    if (inpix[achan] == 0)
                        inval = 0;
                    else
                        inval = inpix[c] * 255 / inpix[achan];
                } else
                    inval = inpix[c];

                temp = functions[c] (inval, channels[c]);
                if (temp > 255)
                    temp = 255;
                else if (temp < 0)
                    temp = 0;
                outpix[c] = temp;
            }
            for (c = 0; c < 3; c++)
                output_pixels[y * rowstride + x * 4 + ctx->channelmap[c]] =
                    outpix[ctx->channelmap[c]] * outpix[achan] / 255;
            output_pixels[y * rowstride + x * 4 + achan] = outpix[achan];
        }
    rsvg_filter_store_result (self->result, output, ctx);

    g_object_unref (in);
    g_object_unref (output);
}