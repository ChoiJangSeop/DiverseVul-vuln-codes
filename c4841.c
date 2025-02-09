static void rgb_prepare_row24(VncState *vs, uint8_t *dst, int x, int y,
                              int count)
{
    VncDisplay *vd = vs->vd;
    uint32_t *fbptr;
    uint32_t pix;

    fbptr = (uint32_t *)(vd->server->data + y * ds_get_linesize(vs->ds) +
                         x * ds_get_bytes_per_pixel(vs->ds));

    while (count--) {
        pix = *fbptr++;
        *dst++ = (uint8_t)(pix >> vs->ds->surface->pf.rshift);
        *dst++ = (uint8_t)(pix >> vs->ds->surface->pf.gshift);
        *dst++ = (uint8_t)(pix >> vs->ds->surface->pf.bshift);
    }
}