static void vnc_dpy_update(DisplayChangeListener *dcl,
                           int x, int y, int w, int h)
{
    VncDisplay *vd = container_of(dcl, VncDisplay, dcl);
    struct VncSurface *s = &vd->guest;
    int width = surface_width(vd->ds);
    int height = surface_height(vd->ds);

    /* this is needed this to ensure we updated all affected
     * blocks if x % VNC_DIRTY_PIXELS_PER_BIT != 0 */
    w += (x % VNC_DIRTY_PIXELS_PER_BIT);
    x -= (x % VNC_DIRTY_PIXELS_PER_BIT);

    x = MIN(x, width);
    y = MIN(y, height);
    w = MIN(x + w, width) - x;
    h = MIN(y + h, height);

    for (; y < h; y++) {
        bitmap_set(s->dirty[y], x / VNC_DIRTY_PIXELS_PER_BIT,
                   DIV_ROUND_UP(w, VNC_DIRTY_PIXELS_PER_BIT));
    }
}