static void cirrus_do_copy(CirrusVGAState *s, int dst, int src, int w, int h)
{
    int sx, sy;
    int dx, dy;
    int width, height;
    int depth;
    int notify = 0;

    depth = s->get_bpp((VGAState *)s) / 8;
    s->get_resolution((VGAState *)s, &width, &height);

    /* extra x, y */
    sx = (src % (width * depth)) / depth;
    sy = (src / (width * depth));
    dx = (dst % (width *depth)) / depth;
    dy = (dst / (width * depth));

    /* normalize width */
    w /= depth;

    /* if we're doing a backward copy, we have to adjust
       our x/y to be the upper left corner (instead of the lower
       right corner) */
    if (s->cirrus_blt_dstpitch < 0) {
	sx -= (s->cirrus_blt_width / depth) - 1;
	dx -= (s->cirrus_blt_width / depth) - 1;
	sy -= s->cirrus_blt_height - 1;
	dy -= s->cirrus_blt_height - 1;
    }

    /* are we in the visible portion of memory? */
    if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 &&
	(sx + w) <= width && (sy + h) <= height &&
	(dx + w) <= width && (dy + h) <= height) {
	notify = 1;
    }

    /* make to sure only copy if it's a plain copy ROP */
    if (*s->cirrus_rop != cirrus_bitblt_rop_fwd_src &&
	*s->cirrus_rop != cirrus_bitblt_rop_bkwd_src)
	notify = 0;

    /* we have to flush all pending changes so that the copy
       is generated at the appropriate moment in time */
    if (notify)
	vga_hw_update();

    (*s->cirrus_rop) (s, s->vram_ptr + s->cirrus_blt_dstaddr,
		      s->vram_ptr + s->cirrus_blt_srcaddr,
		      s->cirrus_blt_dstpitch, s->cirrus_blt_srcpitch,
		      s->cirrus_blt_width, s->cirrus_blt_height);

    if (notify)
	s->ds->dpy_copy(s->ds,
			sx, sy, dx, dy,
			s->cirrus_blt_width / depth,
			s->cirrus_blt_height);

    /* we don't have to notify the display that this portion has
       changed since dpy_copy implies this */

    if (!notify)
	cirrus_invalidate_region(s, s->cirrus_blt_dstaddr,
				 s->cirrus_blt_dstpitch, s->cirrus_blt_width,
				 s->cirrus_blt_height);
}