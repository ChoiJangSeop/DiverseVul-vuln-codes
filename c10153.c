expandrow(UINT8 *dest, UINT8 *src, int n, int z, int xsize) {
    UINT8 pixel, count;
    int x = 0;

    for (; n > 0; n--) {
        pixel = *src++;
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
                *dest = *src++;
                dest += z;
            }

        } else {
            pixel = *src++;
            while (count--) {
                *dest = pixel;
                dest += z;
            }
        }
    }
    return 0;
}