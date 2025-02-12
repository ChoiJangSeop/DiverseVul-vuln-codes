static void vnc_dpy_copy(DisplayState *ds, int src_x, int src_y, int dst_x, int dst_y, int w, int h)
{
    VncDisplay *vd = ds->opaque;
    VncState *vs, *vn;
    uint8_t *src_row;
    uint8_t *dst_row;
    int i,x,y,pitch,depth,inc,w_lim,s;
    int cmp_bytes;

    vnc_refresh_server_surface(vd);
    QTAILQ_FOREACH_SAFE(vs, &vd->clients, next, vn) {
        if (vnc_has_feature(vs, VNC_FEATURE_COPYRECT)) {
            vs->force_update = 1;
            vnc_update_client_sync(vs, 1);
            /* vs might be free()ed here */
        }
    }

    /* do bitblit op on the local surface too */
    pitch = ds_get_linesize(vd->ds);
    depth = ds_get_bytes_per_pixel(vd->ds);
    src_row = vd->server->data + pitch * src_y + depth * src_x;
    dst_row = vd->server->data + pitch * dst_y + depth * dst_x;
    y = dst_y;
    inc = 1;
    if (dst_y > src_y) {
        /* copy backwards */
        src_row += pitch * (h-1);
        dst_row += pitch * (h-1);
        pitch = -pitch;
        y = dst_y + h - 1;
        inc = -1;
    }
    w_lim = w - (16 - (dst_x % 16));
    if (w_lim < 0)
        w_lim = w;
    else
        w_lim = w - (w_lim % 16);
    for (i = 0; i < h; i++) {
        for (x = 0; x <= w_lim;
                x += s, src_row += cmp_bytes, dst_row += cmp_bytes) {
            if (x == w_lim) {
                if ((s = w - w_lim) == 0)
                    break;
            } else if (!x) {
                s = (16 - (dst_x % 16));
                s = MIN(s, w_lim);
            } else {
                s = 16;
            }
            cmp_bytes = s * depth;
            if (memcmp(src_row, dst_row, cmp_bytes) == 0)
                continue;
            memmove(dst_row, src_row, cmp_bytes);
            QTAILQ_FOREACH(vs, &vd->clients, next) {
                if (!vnc_has_feature(vs, VNC_FEATURE_COPYRECT)) {
                    set_bit(((x + dst_x) / 16), vs->dirty[y]);
                }
            }
        }
        src_row += pitch - w * depth;
        dst_row += pitch - w * depth;
        y += inc;
    }

    QTAILQ_FOREACH(vs, &vd->clients, next) {
        if (vnc_has_feature(vs, VNC_FEATURE_COPYRECT)) {
            vnc_copy(vs, src_x, src_y, dst_x, dst_y, w, h);
        }
    }
}