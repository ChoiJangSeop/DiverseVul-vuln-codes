static int expandrow(UINT8* dest, UINT8* src, int n, int z)
{
    UINT8 pixel, count;

    for (;n > 0; n--)
    {
        pixel = *src++;
        if (n == 1 && pixel != 0)
            return n;
        count = pixel & RLE_MAX_RUN;
        if (!count)
            return count;
        if (pixel & RLE_COPY_FLAG) {
            while(count--) {
                *dest = *src++;
                dest += z;
            }

        }
        else {
            pixel = *src++;
            while (count--) {
                *dest = pixel;
                dest += z;
            }
        }

    }
    return 0;
}