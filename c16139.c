rsvg_paint_server_parse (gboolean * inherit, const RsvgDefs * defs, const char *str,
                         guint32 current_color)
{
    char *name;
    guint32 rgb;
    if (inherit != NULL)
        *inherit = 1;
    if (!strcmp (str, "none"))
        return NULL;

    name = rsvg_get_url_string (str);
    if (name) {
        RsvgNode *val;
        val = rsvg_defs_lookup (defs, name);
        g_free (name);

        if (val == NULL)
            return NULL;
        if (!strcmp (val->type->str, "linearGradient"))
            return rsvg_paint_server_lin_grad ((RsvgLinearGradient *) val);
        else if (!strcmp (val->type->str, "radialGradient"))
            return rsvg_paint_server_rad_grad ((RsvgRadialGradient *) val);
        else if (!strcmp (val->type->str, "pattern"))
            return rsvg_paint_server_pattern ((RsvgPattern *) val);
        else
            return NULL;
    } else if (!strcmp (str, "inherit")) {
        if (inherit != NULL)
            *inherit = 0;
        return rsvg_paint_server_solid (0);
    } else if (!strcmp (str, "currentColor")) {
        RsvgPaintServer *ps;
        ps = rsvg_paint_server_solid_current_colour ();
        return ps;
    } else {
        rgb = rsvg_css_parse_color (str, inherit);
        return rsvg_paint_server_solid (rgb);
    }
}