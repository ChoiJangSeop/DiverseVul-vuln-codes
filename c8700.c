int vnc_tight_send_framebuffer_update(VncState *vs, int x, int y,
                                      int w, int h)
{
    vs->tight.type = VNC_ENCODING_TIGHT;
    return tight_send_framebuffer_update(vs, x, y, w, h);
}