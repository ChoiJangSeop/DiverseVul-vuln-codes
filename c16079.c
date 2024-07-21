rsvg_new_any_poly (gboolean is_polyline)
{
    RsvgNodePoly *poly;
    poly = g_new (RsvgNodePoly, 1);
    _rsvg_node_init (&poly->super);
    poly->super.free = _rsvg_node_poly_free;
    poly->super.draw = _rsvg_node_poly_draw;
    poly->super.set_atts = _rsvg_node_poly_set_atts;
    poly->pointlist = NULL;
    poly->is_polyline = is_polyline;
    poly->pointlist_len = 0;
    return &poly->super;
}