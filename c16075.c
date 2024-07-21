rsvg_new_filter_primitive_turbulence (void)
{
    RsvgFilterPrimitiveTurbulence *filter;
    filter = g_new (RsvgFilterPrimitiveTurbulence, 1);
    _rsvg_node_init (&filter->super.super);
    filter->super.in = g_string_new ("none");
    filter->super.result = g_string_new ("none");
    filter->super.x.factor = filter->super.y.factor = filter->super.width.factor =
        filter->super.height.factor = 'n';
    filter->fBaseFreqX = 0;
    filter->fBaseFreqY = 0;
    filter->nNumOctaves = 1;
    filter->seed = 0;
    filter->bDoStitching = 0;
    filter->bFractalSum = 0;
    feTurbulence_init (filter);
    filter->super.render = &rsvg_filter_primitive_turbulence_render;
    filter->super.super.free = &rsvg_filter_primitive_turbulence_free;
    filter->super.super.set_atts = rsvg_filter_primitive_turbulence_set_atts;
    return (RsvgNode *) filter;
}