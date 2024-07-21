rsvg_new_filter_primitive_blend (void)
{
    RsvgFilterPrimitiveBlend *filter;
    filter = g_new (RsvgFilterPrimitiveBlend, 1);
    _rsvg_node_init (&filter->super.super);
    filter->mode = normal;
    filter->super.in = g_string_new ("none");
    filter->in2 = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->super.render = &rsvg_filter_primitive_blend_render;
    filter->super.super.free = &rsvg_filter_primitive_blend_free;
    filter->super.super.set_atts = rsvg_filter_primitive_blend_set_atts;
    return (RsvgNode *) filter;
}