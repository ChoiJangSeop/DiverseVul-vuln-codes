rsvg_new_radial_gradient (void)
{

    RsvgRadialGradient *grad = g_new (RsvgRadialGradient, 1);
    _rsvg_node_init (&grad->super);
    _rsvg_affine_identity (grad->affine);
    grad->has_current_color = FALSE;
    grad->obj_bbox = TRUE;
    grad->spread = RSVG_GRADIENT_PAD;
    grad->fallback = NULL;
    grad->cx = grad->cy = grad->r = grad->fx = grad->fy = _rsvg_css_parse_length ("0.5");
    grad->super.set_atts = rsvg_radial_gradient_set_atts;
    grad->hascx = grad->hascy = grad->hasfx = grad->hasfy = grad->hasr = grad->hasbbox =
        grad->hasspread = grad->hastransform = FALSE;
    return &grad->super;
}