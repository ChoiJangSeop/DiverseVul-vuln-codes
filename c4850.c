static void vnc_write_pixels_generic(VncState *vs, struct PixelFormat *pf,
                                     void *pixels1, int size)
{
    uint8_t buf[4];

    if (pf->bytes_per_pixel == 4) {
        uint32_t *pixels = pixels1;
        int n, i;
        n = size >> 2;
        for(i = 0; i < n; i++) {
            vnc_convert_pixel(vs, buf, pixels[i]);
            vnc_write(vs, buf, vs->clientds.pf.bytes_per_pixel);
        }
    } else if (pf->bytes_per_pixel == 2) {
        uint16_t *pixels = pixels1;
        int n, i;
        n = size >> 1;
        for(i = 0; i < n; i++) {
            vnc_convert_pixel(vs, buf, pixels[i]);
            vnc_write(vs, buf, vs->clientds.pf.bytes_per_pixel);
        }
    } else if (pf->bytes_per_pixel == 1) {
        uint8_t *pixels = pixels1;
        int n, i;
        n = size;
        for(i = 0; i < n; i++) {
            vnc_convert_pixel(vs, buf, pixels[i]);
            vnc_write(vs, buf, vs->clientds.pf.bytes_per_pixel);
        }
    } else {
        fprintf(stderr, "vnc_write_pixels_generic: VncState color depth not supported\n");
    }
}