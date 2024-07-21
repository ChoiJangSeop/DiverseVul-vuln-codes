rsvg_new_symbol (void)
{
    RsvgNodeSymbol *symbol;
    symbol = g_new (RsvgNodeSymbol, 1);
    _rsvg_node_init (&symbol->super);
    symbol->vbox.active = FALSE;
    symbol->preserve_aspect_ratio = RSVG_ASPECT_RATIO_XMID_YMID;
    symbol->super.draw = _rsvg_node_draw_nothing;
    symbol->super.set_atts = rsvg_node_symbol_set_atts;
    return &symbol->super;
}