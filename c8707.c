static int send_sub_rect_jpeg(VncState *vs, int x, int y, int w, int h,
                              int bg, int fg, int colors,
                              VncPalette *palette, bool force)
{
    int ret;

    if (colors == 0) {
        if (force || (tight_jpeg_conf[vs->tight.quality].jpeg_full &&
                      tight_detect_smooth_image(vs, w, h))) {
            int quality = tight_conf[vs->tight.quality].jpeg_quality;

            ret = send_jpeg_rect(vs, x, y, w, h, quality);
        } else {
            ret = send_full_color_rect(vs, x, y, w, h);
        }
    } else if (colors == 1) {
        ret = send_solid_rect(vs);
    } else if (colors == 2) {
        ret = send_mono_rect(vs, x, y, w, h, bg, fg);
    } else if (colors <= 256) {
        if (force || (colors > 96 &&
                      tight_jpeg_conf[vs->tight.quality].jpeg_idx &&
                      tight_detect_smooth_image(vs, w, h))) {
            int quality = tight_conf[vs->tight.quality].jpeg_quality;

            ret = send_jpeg_rect(vs, x, y, w, h, quality);
        } else {
            ret = send_palette_rect(vs, x, y, w, h, palette);
        }
    } else {
        ret = 0;
    }
    return ret;
}