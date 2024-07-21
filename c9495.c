static bool vgacon_scroll(struct vc_data *c, unsigned int t, unsigned int b,
		enum con_scroll dir, unsigned int lines)
{
	unsigned long oldo;
	unsigned int delta;

	if (t || b != c->vc_rows || vga_is_gfx || c->vc_mode != KD_TEXT)
		return false;

	if (!vga_hardscroll_enabled || lines >= c->vc_rows / 2)
		return false;

	vgacon_restore_screen(c);
	oldo = c->vc_origin;
	delta = lines * c->vc_size_row;
	if (dir == SM_UP) {
		vgacon_scrollback_update(c, t, lines);
		if (c->vc_scr_end + delta >= vga_vram_end) {
			scr_memcpyw((u16 *) vga_vram_base,
				    (u16 *) (oldo + delta),
				    c->vc_screenbuf_size - delta);
			c->vc_origin = vga_vram_base;
			vga_rolled_over = oldo - vga_vram_base;
		} else
			c->vc_origin += delta;
		scr_memsetw((u16 *) (c->vc_origin + c->vc_screenbuf_size -
				     delta), c->vc_video_erase_char,
			    delta);
	} else {
		if (oldo - delta < vga_vram_base) {
			scr_memmovew((u16 *) (vga_vram_end -
					      c->vc_screenbuf_size +
					      delta), (u16 *) oldo,
				     c->vc_screenbuf_size - delta);
			c->vc_origin = vga_vram_end - c->vc_screenbuf_size;
			vga_rolled_over = 0;
		} else
			c->vc_origin -= delta;
		c->vc_scr_end = c->vc_origin + c->vc_screenbuf_size;
		scr_memsetw((u16 *) (c->vc_origin), c->vc_video_erase_char,
			    delta);
	}
	c->vc_scr_end = c->vc_origin + c->vc_screenbuf_size;
	c->vc_visible_origin = c->vc_origin;
	vga_set_mem_top(c);
	c->vc_pos = (c->vc_pos - oldo) + c->vc_origin;
	return true;
}