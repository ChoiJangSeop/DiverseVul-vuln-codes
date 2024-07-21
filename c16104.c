rsvg_filter_primitive_specular_lighting_render (RsvgFilterPrimitive * self, RsvgFilterContext * ctx)
{
    gint x, y;
    gdouble z, surfaceScale;
    gint rowstride, height, width;
    gdouble factor, max, base;
    vector3 lightcolour, colour;
    vector3 L;
    gdouble iaffine[6];
    RsvgIRect boundarys;
    RsvgNodeLightSource *source = NULL;

    guchar *in_pixels;
    guchar *output_pixels;

    RsvgFilterPrimitiveSpecularLighting *upself;

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

    upself = (RsvgFilterPrimitiveSpecularLighting *) self;
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

    _rsvg_affine_invert (iaffine, ctx->paffine);

    for (y = boundarys.y0; y < boundarys.y1; y++)
        for (x = boundarys.x0; x < boundarys.x1; x++) {
            z = in_pixels[y * rowstride + x * 4 + 3] * surfaceScale;
            L = get_light_direction (source, x, y, z, iaffine, ctx->ctx);
            L.z += 1;
            L = normalise (L);

            lightcolour = get_light_colour (source, colour, x, y, z, iaffine, ctx->ctx);
            base = dotproduct (get_surface_normal (in_pixels, boundarys, x, y,
                                                   1, 1, 1.0 / ctx->paffine[0],
                                                   1.0 / ctx->paffine[3], upself->surfaceScale,
                                                   rowstride, ctx->channelmap[3]), L);

            factor = upself->specularConstant * pow (base, upself->specularExponent) * 255;

            max = 0;
            if (max < lightcolour.x)
                max = lightcolour.x;
            if (max < lightcolour.y)
                max = lightcolour.y;
            if (max < lightcolour.z)
                max = lightcolour.z;

            max *= factor;
            if (max > 255)
                max = 255;
            if (max < 0)
                max = 0;

            output_pixels[y * rowstride + x * 4 + ctx->channelmap[0]] = lightcolour.x * max;
            output_pixels[y * rowstride + x * 4 + ctx->channelmap[1]] = lightcolour.y * max;
            output_pixels[y * rowstride + x * 4 + ctx->channelmap[2]] = lightcolour.z * max;
            output_pixels[y * rowstride + x * 4 + ctx->channelmap[3]] = max;

        }

    rsvg_filter_store_result (self->result, output, ctx);

    g_object_unref (in);
    g_object_unref (output);
}