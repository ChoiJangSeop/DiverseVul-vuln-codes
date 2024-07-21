rsvg_new_filter_primitive_flood (void)
{
    RsvgFilterPrimitive *filter;
    filter = g_new (RsvgFilterPrimitive, 1);
    _rsvg_node_init (&filter->super);
    filter->in = g_string_new ("none");
    filter->result = g_string_new ("none");
    filter->x.factor = filter->y.factor = filter->width.factor = filter->height.factor = 'n';
    filter->render = &rsvg_filter_primitive_flood_render;
    filter->super.free = &rsvg_filter_primitive_flood_free;
    filter->super.set_atts = rsvg_filter_primitive_flood_set_atts;
    return (RsvgNode *) filter;
}