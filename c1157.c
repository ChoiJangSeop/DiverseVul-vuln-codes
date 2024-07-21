static void qxl_vga_ioport_write(void *opaque, uint32_t addr, uint32_t val)
{
    VGACommonState *vga = opaque;
    PCIQXLDevice *qxl = container_of(vga, PCIQXLDevice, vga);

    if (qxl->mode != QXL_MODE_VGA) {
        dprint(qxl, 1, "%s\n", __FUNCTION__);
        qxl_destroy_primary(qxl);
        qxl_soft_reset(qxl);
    }
    vga_ioport_write(opaque, addr, val);
}