glue(glue(cirrus_bitblt_rop_bkwd_transp_, ROP_NAME),_8)(CirrusVGAState *s,
                                                        uint32_t dstaddr,
                                                        const uint8_t *src,
                                                        int dstpitch,
                                                        int srcpitch,
                                                        int bltwidth,
                                                        int bltheight)
{
    int x,y;
    uint8_t transp = s->vga.gr[0x34];
    dstpitch += bltwidth;
    srcpitch += bltwidth;
    for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x++) {
            ROP_OP_TR(s, dstaddr, *src, transp);
            dstaddr--;
            src--;
        }
        dstaddr += dstpitch;
        src += srcpitch;
    }
}