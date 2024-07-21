rsvg_new_filter_primitive_diffuse_lighting (void)
{
    RsvgFilterPrimitiveDiffuseLighting *filter;
    filter = g_new (RsvgFilterPrimitiveDiffuseLighting, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.in = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->surfaceScale = 1;
    filter->diffuseConstant = 1;
    filter->dx = 1;
    filter->dy = 1;
    filter->lightingcolour = 0xFFFFFFFF;
    filter->super.render = &rsvg_filter_primitive_diffuse_lighting_render;
    filter->super.super.free = &rsvg_filter_primitive_diffuse_lighting_free;
    filter->super.super.set_atts = rsvg_filter_primitive_diffuse_lighting_set_atts;
    return (RsvgNode *) filter;
}