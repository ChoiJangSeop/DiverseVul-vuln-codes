void qemu_spice_create_host_primary(SimpleSpiceDisplay *ssd)
{
    QXLDevSurfaceCreate surface;

    dprint(1, "%s: %dx%d\n", __FUNCTION__,
           ds_get_width(ssd->ds), ds_get_height(ssd->ds));

    surface.format     = SPICE_SURFACE_FMT_32_xRGB;
    surface.width      = ds_get_width(ssd->ds);
    surface.height     = ds_get_height(ssd->ds);
    surface.stride     = -surface.width * 4;
    surface.mouse_mode = true;
    surface.flags      = 0;
    surface.type       = 0;
    surface.mem        = (intptr_t)ssd->buf;
    surface.group_id   = MEMSLOT_GROUP_HOST;

    qemu_spice_create_primary_surface(ssd, 0, &surface);
}