rsvg_new_marker (void)
{
    RsvgMarker *marker;
    marker = g_new (RsvgMarker, 1);
    _rsvg_node_init (&marker->super);
    marker->orient = 0;
    marker->orientAuto = FALSE;
    marker->preserve_aspect_ratio = RSVG_ASPECT_RATIO_XMID_YMID;
    marker->refX = marker->refY = _rsvg_css_parse_length ("0");
    marker->width = marker->height = _rsvg_css_parse_length ("1");
    marker->bbox = TRUE;
    marker->vbox.active = FALSE;
    marker->super.set_atts = rsvg_node_marker_set_atts;
    return &marker->super;
}