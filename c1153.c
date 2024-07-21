static void qxl_destroy_primary(PCIQXLDevice *d)
{
    if (d->mode == QXL_MODE_UNDEFINED) {
        return;
    }

    dprint(d, 1, "%s\n", __FUNCTION__);

    d->mode = QXL_MODE_UNDEFINED;
    qemu_spice_destroy_primary_surface(&d->ssd, 0);
}