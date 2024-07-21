_rsvg_node_poly_draw (RsvgNode * self, RsvgDrawingCtx * ctx, int dominate)
{
    RsvgNodePoly *poly = (RsvgNodePoly *) self;
    gsize i;
    GString *d;
    char buf[G_ASCII_DTOSTR_BUF_SIZE];

    /* represent as a "moveto, lineto*, close" path */
    if (poly->pointlist_len < 2)
        return;

    d = g_string_new (NULL);

    /*      "M %f %f " */
    g_string_append (d, " M ");
    g_string_append (d, g_ascii_dtostr (buf, sizeof (buf), poly->pointlist[0]));
    g_string_append_c (d, ' ');
    g_string_append (d, g_ascii_dtostr (buf, sizeof (buf), poly->pointlist[1]));

    /* "L %f %f " */
    for (i = 2; i < poly->pointlist_len; i += 2) {
        g_string_append (d, " L ");
        g_string_append (d, g_ascii_dtostr (buf, sizeof (buf), poly->pointlist[i]));
        g_string_append_c (d, ' ');
        g_string_append (d, g_ascii_dtostr (buf, sizeof (buf), poly->pointlist[i + 1]));
    }

    if (!poly->is_polyline)
        g_string_append (d, " Z");

    rsvg_state_reinherit_top (ctx, self->state, dominate);
    rsvg_render_path (ctx, d->str);

    g_string_free (d, TRUE);
}