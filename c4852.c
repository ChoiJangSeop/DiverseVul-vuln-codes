static void set_pixel_format(VncState *vs,
                             int bits_per_pixel, int depth,
                             int big_endian_flag, int true_color_flag,
                             int red_max, int green_max, int blue_max,
                             int red_shift, int green_shift, int blue_shift)
{
    if (!true_color_flag) {
        vnc_client_error(vs);
        return;
    }

    vs->clientds = *(vs->vd->guest.ds);
    vs->clientds.pf.rmax = red_max;
    vs->clientds.pf.rbits = hweight_long(red_max);
    vs->clientds.pf.rshift = red_shift;
    vs->clientds.pf.rmask = red_max << red_shift;
    vs->clientds.pf.gmax = green_max;
    vs->clientds.pf.gbits = hweight_long(green_max);
    vs->clientds.pf.gshift = green_shift;
    vs->clientds.pf.gmask = green_max << green_shift;
    vs->clientds.pf.bmax = blue_max;
    vs->clientds.pf.bbits = hweight_long(blue_max);
    vs->clientds.pf.bshift = blue_shift;
    vs->clientds.pf.bmask = blue_max << blue_shift;
    vs->clientds.pf.bits_per_pixel = bits_per_pixel;
    vs->clientds.pf.bytes_per_pixel = bits_per_pixel / 8;
    vs->clientds.pf.depth = bits_per_pixel == 32 ? 24 : bits_per_pixel;
    vs->clientds.flags = big_endian_flag ? QEMU_BIG_ENDIAN_FLAG : 0x00;

    set_pixel_conversion(vs);

    vga_hw_invalidate();
    vga_hw_update();
}