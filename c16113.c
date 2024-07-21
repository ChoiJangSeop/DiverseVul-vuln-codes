rsvg_new_filter_primitive_convolve_matrix (void)
{
    RsvgFilterPrimitiveConvolveMatrix *filter;
    filter = g_new (RsvgFilterPrimitiveConvolveMatrix, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.in = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->divisor = 0;
    filter->bias = 0;
    filter->dx = 0;
    filter->dy = 0;
    filter->preservealpha = FALSE;
    filter->edgemode = 0;
    filter->super.render = &rsvg_filter_primitive_convolve_matrix_render;
    filter->super.super.free = &rsvg_filter_primitive_convolve_matrix_free;
    filter->super.super.set_atts = rsvg_filter_primitive_convolve_matrix_set_atts;
    return (RsvgNode *) filter;
}