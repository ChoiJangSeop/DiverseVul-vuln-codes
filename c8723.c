static int send_jpeg_rect(VncState *vs, int x, int y, int w, int h, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_destination_mgr manager;
    pixman_image_t *linebuf;
    JSAMPROW row[1];
    uint8_t *buf;
    int dy;

    if (surface_bytes_per_pixel(vs->vd->ds) == 1) {
        return send_full_color_rect(vs, x, y, w, h);
    }

    buffer_reserve(&vs->tight.jpeg, 2048);

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    cinfo.client_data = vs;
    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, true);

    manager.init_destination = jpeg_init_destination;
    manager.empty_output_buffer = jpeg_empty_output_buffer;
    manager.term_destination = jpeg_term_destination;
    cinfo.dest = &manager;

    jpeg_start_compress(&cinfo, true);

    linebuf = qemu_pixman_linebuf_create(PIXMAN_BE_r8g8b8, w);
    buf = (uint8_t *)pixman_image_get_data(linebuf);
    row[0] = buf;
    for (dy = 0; dy < h; dy++) {
        qemu_pixman_linebuf_fill(linebuf, vs->vd->server, w, x, y + dy);
        jpeg_write_scanlines(&cinfo, row, 1);
    }
    qemu_pixman_image_unref(linebuf);

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    vnc_write_u8(vs, VNC_TIGHT_JPEG << 4);

    tight_send_compact_size(vs, vs->tight.jpeg.offset);
    vnc_write(vs, vs->tight.jpeg.buffer, vs->tight.jpeg.offset);
    buffer_reset(&vs->tight.jpeg);

    return 1;
}