rsvg_filter_primitive_diffuse_lighting_render (RsvgFilterPrimitive * self, RsvgFilterContext * ctx)
{
    gint x, y;
    float dy, dx, rawdy, rawdx;
    gdouble z;
    gint rowstride, height, width;
    gdouble factor, surfaceScale;
    vector3 lightcolour, L, N;
    vector3 colour;
    gdouble iaffine[6];
    RsvgNodeLightSource *source = NULL;
    RsvgIRect boundarys;

    guchar *in_pixels;
    guchar *output_pixels;

    RsvgFilterPrimitiveDiffuseLighting *upself;

    GdkPixbuf *output;
    GdkPixbuf *in;
    unsigned int i;

    for (i = 0; i < self->super.children->len; i++) {
        RsvgNode *temp;
        temp = g_ptr_array_index (self->super.children, i);
        if (!strcmp (temp->type->str, "feDistantLight") ||
            !strcmp (temp->type->str, "fePointLight") || !strcmp (temp->type->str, "feSpotLight"))
            source = (RsvgNodeLightSource *) temp;
    }
    if (source == NULL)
        return;

    upself = (RsvgFilterPrimitiveDiffuseLighting *) self;
    boundarys = rsvg_filter_primitive_get_bounds (self, ctx);

    in = rsvg_filter_get_in (self->in, ctx);
    in_pixels = gdk_pixbuf_get_pixels (in);

    height = gdk_pixbuf_get_height (in);
    width = gdk_pixbuf_get_width (in);

    rowstride = gdk_pixbuf_get_rowstride (in);

    output = _rsvg_pixbuf_new_cleared (GDK_COLORSPACE_RGB, 1, 8, width, height);

    output_pixels = gdk_pixbuf_get_pixels (output);

    colour.x = ((guchar *) (&upself->lightingcolour))[2] / 255.0;
    colour.y = ((guchar *) (&upself->lightingcolour))[1] / 255.0;
    colour.z = ((guchar *) (&upself->lightingcolour))[0] / 255.0;

    surfaceScale = upself->surfaceScale / 255.0;

    if (upself->dy < 0 || upself->dx < 0) {
        dx = 1;
        dy = 1;
        rawdx = 1;
        rawdy = 1;
    } else {
        dx = upself->dx * ctx->paffine[0];
        dy = upself->dy * ctx->paffine[3];
        rawdx = upself->dx;
        rawdy = upself->dy;
    }

    _rsvg_affine_invert (iaffine, ctx->paffine);

    for (y = boundarys.y0; y < boundarys.y1; y++)
        for (x = boundarys.x0; x < boundarys.x1; x++) {
            z = surfaceScale * (double) in_pixels[y * rowstride + x * 4 + ctx->channelmap[3]];
            L = get_light_direction (source, x, y, z, iaffine, ctx->ctx);
            N = get_surface_normal (in_pixels, boundarys, x, y,
                                    dx, dy, rawdx, rawdy, upself->surfaceScale,
                                    rowstride, ctx->channelmap[3]);
            lightcolour = get_light_colour (source, colour, x, y, z, iaffine, ctx->ctx);
            factor = dotproduct (N, L);

            output_pixels[y * rowstride + x * 4 + ctx->channelmap[0]] =
                MAX (0, MIN (255, upself->diffuseConstant * factor * lightcolour.x * 255.0));
            output_pixels[y * rowstride + x * 4 + ctx->channelmap[1]] =
                MAX (0, MIN (255, upself->diffuseConstant * factor * lightcolour.y * 255.0));
            output_pixels[y * rowstride + x * 4 + ctx->channelmap[2]] =
                MAX (0, MIN (255, upself->diffuseConstant * factor * lightcolour.z * 255.0));
            output_pixels[y * rowstride + x * 4 + ctx->channelmap[3]] = 255;
        }

    rsvg_filter_store_result (self->result, output, ctx);

    g_object_unref (in);
    g_object_unref (output);
}