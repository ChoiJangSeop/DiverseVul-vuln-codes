rsvg_new_filter_primitive_merge_node (void)
{
    RsvgFilterPrimitive *filter;
    filter = g_new (RsvgFilterPrimitive, 1);
    _rsvg_node_init (&filter->super);
    filter->in = g_string_new ("none");
    filter->super.free = rsvg_filter_primitive_merge_node_free;
    filter->render = &rsvg_filter_primitive_merge_node_render;
    filter->super.set_atts = rsvg_filter_primitive_merge_node_set_atts;
    return (RsvgNode *) filter;
}