ImagingConvertTransparent(Imaging imIn, const char *mode, int r, int g, int b) {
    ImagingSectionCookie cookie;
    ImagingShuffler convert;
    Imaging imOut = NULL;
    int y;

    if (!imIn) {
        return (Imaging)ImagingError_ModeError();
    }

    if (!((strcmp(imIn->mode, "RGB") == 0 || strcmp(imIn->mode, "1") == 0 ||
           strcmp(imIn->mode, "I") == 0 || strcmp(imIn->mode, "L") == 0) &&
          strcmp(mode, "RGBA") == 0))
#ifdef notdef
    {
        return (Imaging)ImagingError_ValueError("conversion not supported");
    }
#else
    {
        static char buf[256];
        /* FIXME: may overflow if mode is too large */
        sprintf(
            buf,
            "conversion from %s to %s not supported in convert_transparent",
            imIn->mode,
            mode);
        return (Imaging)ImagingError_ValueError(buf);
    }
#endif

    if (strcmp(imIn->mode, "RGB") == 0) {
        convert = rgb2rgba;
    } else {
        if (strcmp(imIn->mode, "1") == 0) {
            convert = bit2rgb;
        } else if (strcmp(imIn->mode, "I") == 0) {
            convert = i2rgb;
        } else {
            convert = l2rgb;
        }
        g = b = r;
    }

    imOut = ImagingNew2Dirty(mode, imOut, imIn);
    if (!imOut) {
        return NULL;
    }

    ImagingSectionEnter(&cookie);
    for (y = 0; y < imIn->ysize; y++) {
        (*convert)((UINT8 *)imOut->image[y], (UINT8 *)imIn->image[y], imIn->xsize);
        rgbT2rgba((UINT8 *)imOut->image[y], imIn->xsize, r, g, b);
    }
    ImagingSectionLeave(&cookie);

    return imOut;
}