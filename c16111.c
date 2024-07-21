rsvg_new_tspan (void)
{
    RsvgNodeText *text;
    text = g_new (RsvgNodeText, 1);
    _rsvg_node_init (&text->super);
    text->super.set_atts = _rsvg_node_tspan_set_atts;
    text->x.factor = text->y.factor = 'n';
    text->dx = text->dy = _rsvg_css_parse_length ("0");
    return &text->super;
}