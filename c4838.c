static int vnc_refresh_server_surface(VncDisplay *vd)
{
    int y;
    uint8_t *guest_row;
    uint8_t *server_row;
    int cmp_bytes;
    VncState *vs;
    int has_dirty = 0;

    struct timeval tv = { 0, 0 };

    if (!vd->non_adaptive) {
        gettimeofday(&tv, NULL);
        has_dirty = vnc_update_stats(vd, &tv);
    }

    /*
     * Walk through the guest dirty map.
     * Check and copy modified bits from guest to server surface.
     * Update server dirty map.
     */
    cmp_bytes = 16 * ds_get_bytes_per_pixel(vd->ds);
    if (cmp_bytes > vd->ds->surface->linesize) {
        cmp_bytes = vd->ds->surface->linesize;
    }
    guest_row  = vd->guest.ds->data;
    server_row = vd->server->data;
    for (y = 0; y < vd->guest.ds->height; y++) {
        if (!bitmap_empty(vd->guest.dirty[y], VNC_DIRTY_BITS)) {
            int x;
            uint8_t *guest_ptr;
            uint8_t *server_ptr;

            guest_ptr  = guest_row;
            server_ptr = server_row;

            for (x = 0; x + 15 < vd->guest.ds->width;
                    x += 16, guest_ptr += cmp_bytes, server_ptr += cmp_bytes) {
                if (!test_and_clear_bit((x / 16), vd->guest.dirty[y]))
                    continue;
                if (memcmp(server_ptr, guest_ptr, cmp_bytes) == 0)
                    continue;
                memcpy(server_ptr, guest_ptr, cmp_bytes);
                if (!vd->non_adaptive)
                    vnc_rect_updated(vd, x, y, &tv);
                QTAILQ_FOREACH(vs, &vd->clients, next) {
                    set_bit((x / 16), vs->dirty[y]);
                }
                has_dirty++;
            }
        }
        guest_row  += ds_get_linesize(vd->ds);
        server_row += ds_get_linesize(vd->ds);
    }
    return has_dirty;
}