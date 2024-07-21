rsvg_new_image (void)
{
    RsvgNodeImage *image;
    image = g_new (RsvgNodeImage, 1);
    _rsvg_node_init (&image->super);
    g_assert (image->super.state);
    image->img = NULL;
    image->preserve_aspect_ratio = RSVG_ASPECT_RATIO_XMID_YMID;
    image->x = image->y = image->w = image->h = _rsvg_css_parse_length ("0");
    image->super.free = rsvg_node_image_free;
    image->super.draw = rsvg_node_image_draw;
    image->super.set_atts = rsvg_node_image_set_atts;
    return &image->super;
}