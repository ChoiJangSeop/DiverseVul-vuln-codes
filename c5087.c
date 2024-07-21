glue(cirrus_bitblt_rop_fwd_, ROP_NAME)(CirrusVGAState *s,
                                       uint32_t dstaddr,
                                       const uint8_t *src,
                                       int dstpitch, int srcpitch,
                                       int bltwidth, int bltheight)
{
    int x,y;
    dstpitch -= bltwidth;
    srcpitch -= bltwidth;

    if (bltheight > 1 && (dstpitch < 0 || srcpitch < 0)) {
        return;
    }

    for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x++) {
            ROP_OP(s, dstaddr, *src);
            dstaddr++;
            src++;
        }
        dstaddr += dstpitch;
        src += srcpitch;
    }
}