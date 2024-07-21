static void vnc_write_pixels_copy(VncState *vs, struct PixelFormat *pf,
                                  void *pixels, int size)
{
    vnc_write(vs, pixels, size);
}