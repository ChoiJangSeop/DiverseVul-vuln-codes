tight_filter_gradient24(VncState *vs, uint8_t *buf, int w, int h)
{
    uint32_t *buf32;
    uint32_t pix32;
    int shift[3];
    int *prev;
    int here[3], upper[3], left[3], upperleft[3];
    int prediction;
    int x, y, c;

    buf32 = (uint32_t *)buf;
    memset(vs->tight.gradient.buffer, 0, w * 3 * sizeof(int));

    if ((vs->clientds.flags & QEMU_BIG_ENDIAN_FLAG) ==
        (vs->ds->surface->flags & QEMU_BIG_ENDIAN_FLAG)) {
        shift[0] = vs->clientds.pf.rshift;
        shift[1] = vs->clientds.pf.gshift;
        shift[2] = vs->clientds.pf.bshift;
    } else {
        shift[0] = 24 - vs->clientds.pf.rshift;
        shift[1] = 24 - vs->clientds.pf.gshift;
        shift[2] = 24 - vs->clientds.pf.bshift;
    }

    for (y = 0; y < h; y++) {
        for (c = 0; c < 3; c++) {
            upper[c] = 0;
            here[c] = 0;
        }
        prev = (int *)vs->tight.gradient.buffer;
        for (x = 0; x < w; x++) {
            pix32 = *buf32++;
            for (c = 0; c < 3; c++) {
                upperleft[c] = upper[c];
                left[c] = here[c];
                upper[c] = *prev;
                here[c] = (int)(pix32 >> shift[c] & 0xFF);
                *prev++ = here[c];

                prediction = left[c] + upper[c] - upperleft[c];
                if (prediction < 0) {
                    prediction = 0;
                } else if (prediction > 0xFF) {
                    prediction = 0xFF;
                }
                *buf++ = (char)(here[c] - prediction);
            }
        }
    }
}