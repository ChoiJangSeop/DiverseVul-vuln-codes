rsvg_new_mask (void)
{
    RsvgMask *mask;

    mask = g_new (RsvgMask, 1);
    _rsvg_node_init (&mask->super);
    mask->maskunits = objectBoundingBox;
    mask->contentunits = userSpaceOnUse;
    mask->x = _rsvg_css_parse_length ("0");
    mask->y = _rsvg_css_parse_length ("0");
    mask->width = _rsvg_css_parse_length ("1");
    mask->height = _rsvg_css_parse_length ("1");
    mask->super.set_atts = rsvg_mask_set_atts;
    return &mask->super;
}