rsvg_new_pattern (void)
{
    RsvgPattern *pattern = g_new (RsvgPattern, 1);
    _rsvg_node_init (&pattern->super);
    pattern->obj_bbox = TRUE;
    pattern->obj_cbbox = FALSE;
    pattern->x = pattern->y = pattern->width = pattern->height = _rsvg_css_parse_length ("0");
    pattern->fallback = NULL;
    pattern->preserve_aspect_ratio = RSVG_ASPECT_RATIO_XMID_YMID;
    pattern->vbox.active = FALSE;
    _rsvg_affine_identity (pattern->affine);
    pattern->super.set_atts = rsvg_pattern_set_atts;
    pattern->hasx = pattern->hasy = pattern->haswidth = pattern->hasheight = pattern->hasbbox =
        pattern->hascbox = pattern->hasvbox = pattern->hasaspect = pattern->hastransform = FALSE;
    return &pattern->super;
}