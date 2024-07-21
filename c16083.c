rsvg_new_tref (void)
{
    RsvgNodeTref *text;
    text = g_new (RsvgNodeTref, 1);
    _rsvg_node_init (&text->super);
    text->super.set_atts = _rsvg_node_tref_set_atts;
    text->link = NULL;
    return &text->super;
}