rsvg_linear_gradient_fix_fallback (RsvgLinearGradient * grad)
{
    RsvgNode *ufallback;
    int i;
    ufallback = grad->fallback;
    while (ufallback != NULL) {
        if (!strcmp (ufallback->type->str, "linearGradient")) {
            RsvgLinearGradient *fallback = (RsvgLinearGradient *) ufallback;
            if (!grad->hasx1 && fallback->hasx1) {
                grad->hasx1 = TRUE;
                grad->x1 = fallback->x1;
            }
            if (!grad->hasy1 && fallback->hasy1) {
                grad->hasy1 = TRUE;
                grad->y1 = fallback->y1;
            }
            if (!grad->hasx2 && fallback->hasx2) {
                grad->hasx2 = TRUE;
                grad->x2 = fallback->x2;
            }
            if (!grad->hasy2 && fallback->hasy2) {
                grad->hasy2 = TRUE;
                grad->y2 = fallback->y2;
            }
            if (!grad->hastransform && fallback->hastransform) {
                grad->hastransform = TRUE;
                for (i = 0; i < 6; i++)
                    grad->affine[i] = fallback->affine[i];
            }
            if (!grad->hasspread && fallback->hasspread) {
                grad->hasspread = TRUE;
                grad->spread = fallback->spread;
            }
            if (!grad->hasbbox && fallback->hasbbox) {
                grad->hasbbox = TRUE;
                grad->obj_bbox = fallback->obj_bbox;
            }
            if (!hasstop (grad->super.children) && hasstop (fallback->super.children)) {
                grad->super.children = fallback->super.children;
            }
            ufallback = fallback->fallback;
        } else if (!strcmp (ufallback->type->str, "radialGradient")) {
            RsvgRadialGradient *fallback = (RsvgRadialGradient *) ufallback;
            if (!grad->hastransform && fallback->hastransform) {
                grad->hastransform = TRUE;
                for (i = 0; i < 6; i++)
                    grad->affine[i] = fallback->affine[i];
            }
            if (!grad->hasspread && fallback->hasspread) {
                grad->hasspread = TRUE;
                grad->spread = fallback->spread;
            }
            if (!grad->hasbbox && fallback->hasbbox) {
                grad->hasbbox = TRUE;
                grad->obj_bbox = fallback->obj_bbox;
            }
            if (!hasstop (grad->super.children) && hasstop (fallback->super.children)) {
                grad->super.children = fallback->super.children;
            }
            ufallback = fallback->fallback;
        }
    }
}