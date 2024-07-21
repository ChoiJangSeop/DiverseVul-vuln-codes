static void qxl_create_guest_primary(PCIQXLDevice *qxl, int loadvm)
{
    QXLDevSurfaceCreate surface;
    QXLSurfaceCreate *sc = &qxl->guest_primary.surface;

    assert(qxl->mode != QXL_MODE_NATIVE);
    qxl_exit_vga_mode(qxl);

    dprint(qxl, 1, "%s: %dx%d\n", __FUNCTION__,
           le32_to_cpu(sc->width), le32_to_cpu(sc->height));

    surface.format     = le32_to_cpu(sc->format);
    surface.height     = le32_to_cpu(sc->height);
    surface.mem        = le64_to_cpu(sc->mem);
    surface.position   = le32_to_cpu(sc->position);
    surface.stride     = le32_to_cpu(sc->stride);
    surface.width      = le32_to_cpu(sc->width);
    surface.type       = le32_to_cpu(sc->type);
    surface.flags      = le32_to_cpu(sc->flags);

    surface.mouse_mode = true;
    surface.group_id   = MEMSLOT_GROUP_GUEST;
    if (loadvm) {
        surface.flags |= QXL_SURF_FLAG_KEEP_DATA;
    }

    qxl->mode = QXL_MODE_NATIVE;
    qxl->cmdflags = 0;
    qemu_spice_create_primary_surface(&qxl->ssd, 0, &surface);

    /* for local rendering */
    qxl_render_resize(qxl);
}