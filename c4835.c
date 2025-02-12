static int vnc_cursor_define(VncState *vs)
{
    QEMUCursor *c = vs->vd->cursor;
    PixelFormat pf = qemu_default_pixelformat(32);
    int isize;

    if (vnc_has_feature(vs, VNC_FEATURE_RICH_CURSOR)) {
        vnc_lock_output(vs);
        vnc_write_u8(vs,  VNC_MSG_SERVER_FRAMEBUFFER_UPDATE);
        vnc_write_u8(vs,  0);  /*  padding     */
        vnc_write_u16(vs, 1);  /*  # of rects  */
        vnc_framebuffer_update(vs, c->hot_x, c->hot_y, c->width, c->height,
                               VNC_ENCODING_RICH_CURSOR);
        isize = c->width * c->height * vs->clientds.pf.bytes_per_pixel;
        vnc_write_pixels_generic(vs, &pf, c->data, isize);
        vnc_write(vs, vs->vd->cursor_mask, vs->vd->cursor_msize);
        vnc_unlock_output(vs);
        return 0;
    }
    return -1;
}