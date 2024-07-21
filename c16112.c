_pattern_add_rsvg_color_stops (cairo_pattern_t * pattern,
                               GPtrArray * stops, guint32 current_color_rgb, guint8 opacity)
{
    gsize i;
    RsvgGradientStop *stop;
    RsvgNode *node;
    guint32 rgba;

    for (i = 0; i < stops->len; i++) {
        node = (RsvgNode *) g_ptr_array_index (stops, i);
        if (strcmp (node->type->str, "stop"))
            continue;
        stop = (RsvgGradientStop *) node;
        rgba = stop->rgba;
        cairo_pattern_add_color_stop_rgba (pattern, stop->offset,
                                           ((rgba >> 24) & 0xff) / 255.0,
                                           ((rgba >> 16) & 0xff) / 255.0,
                                           ((rgba >> 8) & 0xff) / 255.0,
                                           (((rgba >> 0) & 0xff) * opacity) / 255.0 / 255.0);
    }
}