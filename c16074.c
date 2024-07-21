rsvg_new_filter_primitive_composite (void)
{
    RsvgFilterPrimitiveComposite *filter;
    filter = g_new (RsvgFilterPrimitiveComposite, 1);
    _rsvg_node_init (&filter->super.super);
    filter->mode = COMPOSITE_MODE_OVER;
    filter->super.in = g_string_new ("none");
    filter->in2 = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->k1 = 0;
    filter->k2 = 0;
    filter->k3 = 0;
    filter->k4 = 0;
    filter->super.render = &rsvg_filter_primitive_composite_render;
    filter->super.super.free = &rsvg_filter_primitive_composite_free;
    filter->super.super.set_atts = rsvg_filter_primitive_composite_set_atts;
    return (RsvgNode *) filter;
}