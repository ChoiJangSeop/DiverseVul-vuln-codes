rsvg_new_filter_primitive_offset (void)
{
    RsvgFilterPrimitiveOffset *filter;
    filter = g_new (RsvgFilterPrimitiveOffset, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.in = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->dy = _rsvg_css_parse_length ("0");
    filter->dx = _rsvg_css_parse_length ("0");
    filter->super.render = &rsvg_filter_primitive_offset_render;
    filter->super.super.free = &rsvg_filter_primitive_offset_free;
    filter->super.super.set_atts = rsvg_filter_primitive_offset_set_atts;
    return (RsvgNode *) filter;
}