tight_detect_smooth_image24(VncState *vs, int w, int h)
{
    int off;
    int x, y, d, dx;
    unsigned int c;
    unsigned int stats[256];
    int pixels = 0;
    int pix, left[3];
    unsigned int errors;
    unsigned char *buf = vs->tight.tight.buffer;

    /*
     * If client is big-endian, color samples begin from the second
     * byte (offset 1) of a 32-bit pixel value.
     */
    off = !!(vs->clientds.flags & QEMU_BIG_ENDIAN_FLAG);

    memset(stats, 0, sizeof (stats));

    for (y = 0, x = 0; y < h && x < w;) {
        for (d = 0; d < h - y && d < w - x - VNC_TIGHT_DETECT_SUBROW_WIDTH;
             d++) {
            for (c = 0; c < 3; c++) {
                left[c] = buf[((y+d)*w+x+d)*4+off+c] & 0xFF;
            }
            for (dx = 1; dx <= VNC_TIGHT_DETECT_SUBROW_WIDTH; dx++) {
                for (c = 0; c < 3; c++) {
                    pix = buf[((y+d)*w+x+d+dx)*4+off+c] & 0xFF;
                    stats[abs(pix - left[c])]++;
                    left[c] = pix;
                }
                pixels++;
            }
        }
        if (w > h) {
            x += h;
            y = 0;
        } else {
            x = 0;
            y += w;
        }
    }

    /* 95% smooth or more ... */
    if (stats[0] * 33 / pixels >= 95) {
        return 0;
    }

    errors = 0;
    for (c = 1; c < 8; c++) {
        errors += stats[c] * (c * c);
        if (stats[c] == 0 || stats[c] > stats[c-1] * 2) {
            return 0;
        }
    }
    for (; c < 256; c++) {
        errors += stats[c] * (c * c);
    }
    errors /= (pixels * 3 - stats[0]);

    return errors;
}