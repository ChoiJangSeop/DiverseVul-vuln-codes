static void qxl_reset_surfaces(PCIQXLDevice *d)
{
    dprint(d, 1, "%s:\n", __FUNCTION__);
    d->mode = QXL_MODE_UNDEFINED;
    qxl_spice_destroy_surfaces(d);
}