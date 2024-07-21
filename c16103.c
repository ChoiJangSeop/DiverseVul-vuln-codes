rsvg_new_filter (void)
{
    RsvgFilter *filter;

    filter = g_new (RsvgFilter, 1);
    _rsvg_node_init (&filter->super);
    filter->filterunits = objectBoundingBox;
    filter->primitiveunits = userSpaceOnUse;
    filter->x = _rsvg_css_parse_length ("-10%");
    filter->y = _rsvg_css_parse_length ("-10%");
    filter->width = _rsvg_css_parse_length ("120%");
    filter->height = _rsvg_css_parse_length ("120%");
    filter->super.set_atts = rsvg_filter_set_args;
    return (RsvgNode *) filter;
}