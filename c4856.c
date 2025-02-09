int vnc_raw_send_framebuffer_update(VncState *vs, int x, int y, int w, int h)
{
    int i;
    uint8_t *row;
    VncDisplay *vd = vs->vd;

    row = vd->server->data + y * ds_get_linesize(vs->ds) + x * ds_get_bytes_per_pixel(vs->ds);
    for (i = 0; i < h; i++) {
        vs->write_pixels(vs, &vd->server->pf, row, w * ds_get_bytes_per_pixel(vs->ds));
        row += ds_get_linesize(vs->ds);
    }
    return 1;
}