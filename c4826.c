static int send_solid_rect(VncState *vs)
{
    size_t bytes;

    vnc_write_u8(vs, VNC_TIGHT_FILL << 4); /* no flushing, no filter */

    if (vs->tight.pixel24) {
        tight_pack24(vs, vs->tight.tight.buffer, 1, &vs->tight.tight.offset);
        bytes = 3;
    } else {
        bytes = vs->clientds.pf.bytes_per_pixel;
    }

    vnc_write(vs, vs->tight.tight.buffer, bytes);
    return 1;
}