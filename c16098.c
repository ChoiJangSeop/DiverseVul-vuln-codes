rsvg_new_filter_primitive_erode (void)
{
    RsvgFilterPrimitiveErode *filter;
    filter = g_new (RsvgFilterPrimitiveErode, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.in = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->rx = 0;
    filter->ry = 0;
    filter->mode = 0;
    filter->super.render = &rsvg_filter_primitive_erode_render;
    filter->super.super.free = &rsvg_filter_primitive_erode_free;
    filter->super.super.set_atts = rsvg_filter_primitive_erode_set_atts;
    return (RsvgNode *) filter;
}