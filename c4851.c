static void vnc_dpy_update(DisplayState *ds, int x, int y, int w, int h)
{
    int i;
    VncDisplay *vd = ds->opaque;
    struct VncSurface *s = &vd->guest;

    h += y;

    /* round x down to ensure the loop only spans one 16-pixel block per,
       iteration.  otherwise, if (x % 16) != 0, the last iteration may span
       two 16-pixel blocks but we only mark the first as dirty
    */
    w += (x % 16);
    x -= (x % 16);

    x = MIN(x, s->ds->width);
    y = MIN(y, s->ds->height);
    w = MIN(x + w, s->ds->width) - x;
    h = MIN(h, s->ds->height);

    for (; y < h; y++)
        for (i = 0; i < w; i += 16)
            set_bit((x + i) / 16, s->dirty[y]);
}