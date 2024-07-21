rsvg_new_svg (void)
{
    RsvgNodeSvg *svg;
    svg = g_new (RsvgNodeSvg, 1);
    _rsvg_node_init (&svg->super);
    svg->vbox.active = FALSE;
    svg->preserve_aspect_ratio = RSVG_ASPECT_RATIO_XMID_YMID;
    svg->x = _rsvg_css_parse_length ("0");
    svg->y = _rsvg_css_parse_length ("0");
    svg->w = _rsvg_css_parse_length ("100%");
    svg->h = _rsvg_css_parse_length ("100%");
    svg->super.draw = rsvg_node_svg_draw;
    svg->super.free = _rsvg_svg_free;
    svg->super.set_atts = rsvg_node_svg_set_atts;
    return &svg->super;
}