rsvg_new_stop (void)
{
    RsvgGradientStop *stop = g_new (RsvgGradientStop, 1);
    _rsvg_node_init (&stop->super);
    stop->super.set_atts = rsvg_stop_set_atts;
    stop->offset = 0;
    stop->rgba = 0;
    return &stop->super;
}