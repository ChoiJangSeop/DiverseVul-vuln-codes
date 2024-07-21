rsvg_new_rect (void)
{
    RsvgNodeRect *rect;
    rect = g_new (RsvgNodeRect, 1);
    _rsvg_node_init (&rect->super);
    rect->super.draw = _rsvg_node_rect_draw;
    rect->super.set_atts = _rsvg_node_rect_set_atts;
    rect->x = rect->y = rect->w = rect->h = rect->rx = rect->ry = _rsvg_css_parse_length ("0");
    rect->got_rx = rect->got_ry = FALSE;
    return &rect->super;
}