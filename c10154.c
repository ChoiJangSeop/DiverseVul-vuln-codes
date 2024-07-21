expandrow2(UINT8 *dest, const UINT8 *src, int n, int z, int xsize) {
    UINT8 pixel, count;

    int x = 0;

    for (; n > 0; n--) {
        pixel = src[1];
        src += 2;
        if (n == 1 && pixel != 0) {
            return n;
        }
        count = pixel & RLE_MAX_RUN;
        if (!count) {
            return count;
        }
        if (x + count > xsize) {
            return -1;
        }
        x += count;
        if (pixel & RLE_COPY_FLAG) {
            while (count--) {
                memcpy(dest, src, 2);
                src += 2;
                dest += z * 2;
            }
        } else {
            while (count--) {
                memcpy(dest, src, 2);
                dest += z * 2;
            }
            src += 2;
        }
    }
    return 0;
}