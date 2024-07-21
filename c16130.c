rsvg_new_filter_primitive_colour_matrix (void)
{
    RsvgFilterPrimitiveColourMatrix *filter;
    filter = g_new (RsvgFilterPrimitiveColourMatrix, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.in = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->KernelMatrix = NULL;
    filter->super.render = &rsvg_filter_primitive_colour_matrix_render;
    filter->super.super.free = &rsvg_filter_primitive_colour_matrix_free;

    filter->super.super.set_atts = rsvg_filter_primitive_colour_matrix_set_atts;
    return (RsvgNode *) filter;
}