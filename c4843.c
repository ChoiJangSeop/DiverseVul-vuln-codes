static bool send_gradient_rect(VncState *vs, int x, int y, int w, int h)
{
    int stream = 3;
    int level = tight_conf[vs->tight.compression].gradient_zlib_level;
    ssize_t bytes;

    if (vs->clientds.pf.bytes_per_pixel == 1)
        return send_full_color_rect(vs, x, y, w, h);

    vnc_write_u8(vs, (stream | VNC_TIGHT_EXPLICIT_FILTER) << 4);
    vnc_write_u8(vs, VNC_TIGHT_FILTER_GRADIENT);

    buffer_reserve(&vs->tight.gradient, w * 3 * sizeof (int));

    if (vs->tight.pixel24) {
        tight_filter_gradient24(vs, vs->tight.tight.buffer, w, h);
        bytes = 3;
    } else if (vs->clientds.pf.bytes_per_pixel == 4) {
        tight_filter_gradient32(vs, (uint32_t *)vs->tight.tight.buffer, w, h);
        bytes = 4;
    } else {
        tight_filter_gradient16(vs, (uint16_t *)vs->tight.tight.buffer, w, h);
        bytes = 2;
    }

    buffer_reset(&vs->tight.gradient);

    bytes = w * h * bytes;
    vs->tight.tight.offset = bytes;

    bytes = tight_compress_data(vs, stream, bytes,
                                level, Z_FILTERED);
    return (bytes >= 0);
}