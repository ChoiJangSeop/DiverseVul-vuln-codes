glue(glue(cirrus_bitblt_rop_bkwd_transp_, ROP_NAME),_16)(CirrusVGAState *s,
                                                         uint32_t dstaddr,
                                                         const uint8_t *src,
                                                         int dstpitch,
                                                         int srcpitch,
                                                         int bltwidth,
                                                         int bltheight)
{
    int x,y;
    uint16_t transp = s->vga.gr[0x34] | (uint16_t)s->vga.gr[0x35] << 8;
    dstpitch += bltwidth;
    srcpitch += bltwidth;
    for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x+=2) {
            ROP_OP_TR_16(s, dstaddr, *(uint16_t *)src, transp);
            dstaddr -= 2;
            src -= 2;
        }
        dstaddr += dstpitch;
        src += srcpitch;
    }
}