glue(glue(cirrus_bitblt_rop_bkwd_transp_, ROP_NAME),_8)(CirrusVGAState *s,
							uint8_t *dst,const uint8_t *src,
							int dstpitch,int srcpitch,
							int bltwidth,int bltheight)
{
    int x,y;
    uint8_t p;
    dstpitch += bltwidth;
    srcpitch += bltwidth;
    for (y = 0; y < bltheight; y++) {
        for (x = 0; x < bltwidth; x++) {
	    p = *dst;
            ROP_OP(&p, *src);
	    if (p != s->vga.gr[0x34]) *dst = p;
            dst--;
            src--;
        }
        dst += dstpitch;
        src += srcpitch;
    }
}