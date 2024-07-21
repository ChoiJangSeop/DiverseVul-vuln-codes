rsvg_new_filter_primitive_specular_lighting (void)
{
    RsvgFilterPrimitiveSpecularLighting *filter;
    filter = g_new (RsvgFilterPrimitiveSpecularLighting, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.in = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->surfaceScale = 1;
    filter->specularConstant = 1;
    filter->specularExponent = 1;
    filter->lightingcolour = 0xFFFFFFFF;
    filter->super.render = &rsvg_filter_primitive_specular_lighting_render;
    filter->super.super.free = &rsvg_filter_primitive_specular_lighting_free;
    filter->super.super.set_atts = rsvg_filter_primitive_specular_lighting_set_atts;
    return (RsvgNode *) filter;
}