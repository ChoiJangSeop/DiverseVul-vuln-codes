pattern_apply_fallback (gpointer data, gpointer fallback_data)
{
    RsvgPattern *pattern;
    RsvgPattern *fallback;

    pattern = data;
    fallback = fallback_data;

    if (!pattern->hasx && fallback->hasx) {
        pattern->hasx = TRUE;
        pattern->x = fallback->x;
    }
    if (!pattern->hasy && fallback->hasy) {
        pattern->hasy = TRUE;
        pattern->y = fallback->y;
    }
    if (!pattern->haswidth && fallback->haswidth) {
        pattern->haswidth = TRUE;
        pattern->width = fallback->width;
    }
    if (!pattern->hasheight && fallback->hasheight) {
        pattern->hasheight = TRUE;
        pattern->height = fallback->height;
    }
    if (!pattern->hastransform && fallback->hastransform) {
        pattern->hastransform = TRUE;
        pattern->affine = fallback->affine;
    }
    if (!pattern->hasvbox && fallback->hasvbox) {
        pattern->vbox = fallback->vbox;
    }
    if (!pattern->hasaspect && fallback->hasaspect) {
        pattern->hasaspect = TRUE;
        pattern->preserve_aspect_ratio = fallback->preserve_aspect_ratio;
    }
    if (!pattern->hasbbox && fallback->hasbbox) {
        pattern->hasbbox = TRUE;
        pattern->obj_bbox = fallback->obj_bbox;
    }
    if (!pattern->hascbox && fallback->hascbox) {
        pattern->hascbox = TRUE;
        pattern->obj_cbbox = fallback->obj_cbbox;
    }
    if (!pattern->super.children->len && fallback->super.children->len) {
        pattern->super.children = fallback->super.children;
    }
}