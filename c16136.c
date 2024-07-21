rsvg_new_filter_primitive_displacement_map (void)
{
    RsvgFilterPrimitiveDisplacementMap *filter;
    filter = g_new (RsvgFilterPrimitiveDisplacementMap, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.in = g_string_new ("none");
    filter->in2 = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->xChannelSelector = ' ';
    filter->yChannelSelector = ' ';
    filter->scale = 0;
    filter->super.render = &rsvg_filter_primitive_displacement_map_render;
    filter->super.super.free = &rsvg_filter_primitive_displacement_map_free;
    filter->super.super.set_atts = rsvg_filter_primitive_displacement_map_set_atts;
    return (RsvgNode *) filter;
}