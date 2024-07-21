rsvg_state_clone (RsvgState * dst, const RsvgState * src)
{
    gint i;
    RsvgState *parent = dst->parent;

    rsvg_state_finalize (dst);

    *dst = *src;
    dst->parent = parent;
    dst->font_family = g_strdup (src->font_family);
    dst->lang = g_strdup (src->lang);
    rsvg_paint_server_ref (dst->fill);
    rsvg_paint_server_ref (dst->stroke);

    dst->styles = g_hash_table_ref (src->styles);

    if (src->dash.n_dash > 0) {
        dst->dash.dash = g_new (gdouble, src->dash.n_dash);
        for (i = 0; i < src->dash.n_dash; i++)
            dst->dash.dash[i] = src->dash.dash[i];
    }
}