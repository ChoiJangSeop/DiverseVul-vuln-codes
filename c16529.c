DECLAREContigPutFunc(putcontig8bitYCbCr42tile)
{
    uint32* cp1 = cp+w+toskew;
    int32 incr = 2*toskew+w;

    (void) y;
    fromskew = (fromskew * 10) / 4;
    if ((h & 3) == 0 && (w & 1) == 0) {
        for (; h >= 2; h -= 2) {
            x = w>>2;
            do {
                int32 Cb = pp[8];
                int32 Cr = pp[9];
                
                YCbCrtoRGB(cp [0], pp[0]);
                YCbCrtoRGB(cp [1], pp[1]);
                YCbCrtoRGB(cp [2], pp[2]);
                YCbCrtoRGB(cp [3], pp[3]);
                YCbCrtoRGB(cp1[0], pp[4]);
                YCbCrtoRGB(cp1[1], pp[5]);
                YCbCrtoRGB(cp1[2], pp[6]);
                YCbCrtoRGB(cp1[3], pp[7]);
                
                cp += 4, cp1 += 4;
                pp += 10;
            } while (--x);
            cp += incr, cp1 += incr;
            pp += fromskew;
        }
    } else {
        while (h > 0) {
            for (x = w; x > 0;) {
                int32 Cb = pp[8];
                int32 Cr = pp[9];
                switch (x) {
                default:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[3], pp[ 7]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [3], pp[ 3]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 3:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[2], pp[ 6]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [2], pp[ 2]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 2:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[1], pp[ 5]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [1], pp[ 1]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 1:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[0], pp[ 4]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [0], pp[ 0]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                }
                if (x < 4) {
                    cp += x; cp1 += x;
                    x = 0;
                }
                else {
                    cp += 4; cp1 += 4;
                    x -= 4;
                }
                pp += 10;
            }
            if (h <= 2)
                break;
            h -= 2;
            cp += incr, cp1 += incr;
            pp += fromskew;
        }
    }
}