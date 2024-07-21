rsvg_new_line (void)
{
    RsvgNodeLine *line;
    line = g_new (RsvgNodeLine, 1);
    _rsvg_node_init (&line->super);
    line->super.draw = _rsvg_node_line_draw;
    line->super.set_atts = _rsvg_node_line_set_atts;
    line->x1 = line->x2 = line->y1 = line->y2 = _rsvg_css_parse_length ("0");
    return &line->super;
}