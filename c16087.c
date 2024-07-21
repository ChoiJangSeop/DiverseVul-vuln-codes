rsvg_new_ellipse (void)
{
    RsvgNodeEllipse *ellipse;
    ellipse = g_new (RsvgNodeEllipse, 1);
    _rsvg_node_init (&ellipse->super);
    ellipse->super.draw = _rsvg_node_ellipse_draw;
    ellipse->super.set_atts = _rsvg_node_ellipse_set_atts;
    ellipse->cx = ellipse->cy = ellipse->rx = ellipse->ry = _rsvg_css_parse_length ("0");
    return &ellipse->super;
}