rsvg_new_circle (void)
{
    RsvgNodeCircle *circle;
    circle = g_new (RsvgNodeCircle, 1);
    _rsvg_node_init (&circle->super);
    circle->super.draw = _rsvg_node_circle_draw;
    circle->super.set_atts = _rsvg_node_circle_set_atts;
    circle->cx = circle->cy = circle->r = _rsvg_css_parse_length ("0");
    return &circle->super;
}