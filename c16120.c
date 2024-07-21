rsvg_new_text (void)
{
    RsvgNodeText *text;
    text = g_new (RsvgNodeText, 1);
    _rsvg_node_init (&text->super);
    text->super.draw = _rsvg_node_text_draw;
    text->super.set_atts = _rsvg_node_text_set_atts;
    text->x = text->y = text->dx = text->dy = _rsvg_css_parse_length ("0");
    return &text->super;
}