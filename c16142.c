rsvg_new_linear_gradient (void)
{
    RsvgLinearGradient *grad = NULL;
    grad = g_new (RsvgLinearGradient, 1);
    _rsvg_node_init (&grad->super);
    _rsvg_affine_identity (grad->affine);
    grad->has_current_color = FALSE;
    grad->x1 = grad->y1 = grad->y2 = _rsvg_css_parse_length ("0");
    grad->x2 = _rsvg_css_parse_length ("1");
    grad->fallback = NULL;
    grad->obj_bbox = TRUE;
    grad->spread = RSVG_GRADIENT_PAD;
    grad->super.set_atts = rsvg_linear_gradient_set_atts;
    grad->hasx1 = grad->hasy1 = grad->hasx2 = grad->hasy2 = grad->hasbbox = grad->hasspread =
        grad->hastransform = FALSE;
    return &grad->super;
}