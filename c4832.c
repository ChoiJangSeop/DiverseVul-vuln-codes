static void vnc_dpy_setdata(DisplayState *ds)
{
    VncDisplay *vd = ds->opaque;

    *(vd->guest.ds) = *(ds->surface);
    vnc_dpy_update(ds, 0, 0, ds_get_width(ds), ds_get_height(ds));
}