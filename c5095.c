glue(cirrus_bitblt_rop_bkwd_, ROP_NAME)(CirrusVGAState *s,
                                        uint32_t dstaddr,
                                        const uint8_t *src,
                                        int dstpitch, int srcpitch,
                                        int bltwidth, int bltheight)
{
    int x,y;
    dstpitch += bltwidth;
    srcpitch += bltwidth;
    for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x++) {
            ROP_OP(s, dstaddr, *src);
            dstaddr--;
            src--;
        }
        dstaddr += dstpitch;
        src += srcpitch;
    }
}