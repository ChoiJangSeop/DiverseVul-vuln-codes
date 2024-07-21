rsvg_new_filter_primitive_merge (void)
{
    RsvgFilterPrimitiveMerge *filter;
    filter = g_new (RsvgFilterPrimitiveMerge, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->super.render = &rsvg_filter_primitive_merge_render;
    filter->super.super.free = &rsvg_filter_primitive_merge_free;

    filter->super.super.set_atts = rsvg_filter_primitive_merge_set_atts;
    return (RsvgNode *) filter;
}