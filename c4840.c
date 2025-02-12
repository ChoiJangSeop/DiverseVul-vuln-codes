static bool tight_can_send_png_rect(VncState *vs, int w, int h)
{
    if (vs->tight.type != VNC_ENCODING_TIGHT_PNG) {
        return false;
    }

    if (ds_get_bytes_per_pixel(vs->ds) == 1 ||
        vs->clientds.pf.bytes_per_pixel == 1) {
        return false;
    }

    return true;
}