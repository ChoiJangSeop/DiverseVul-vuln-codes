rsvg_new_filter_primitive_component_transfer (void)
{
    RsvgFilterPrimitiveComponentTransfer *filter;

    filter = g_new (RsvgFilterPrimitiveComponentTransfer, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.result = g_string_new ("none");
    filter->super.in = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->super.render = &rsvg_filter_primitive_component_transfer_render;

    filter->super.super.set_atts = rsvg_filter_primitive_component_transfer_set_atts;

    return (RsvgNode *) filter;
}