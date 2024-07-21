static void vgacon_restore_screen(struct vc_data *c)
{
	c->vc_origin = c->vc_visible_origin;
	vgacon_scrollback_cur->save = 0;

	if (!vga_is_gfx && !vgacon_scrollback_cur->restore) {
		scr_memcpyw((u16 *) c->vc_origin, (u16 *) c->vc_screenbuf,
			    c->vc_screenbuf_size > vga_vram_size ?
			    vga_vram_size : c->vc_screenbuf_size);
		vgacon_scrollback_cur->restore = 1;
		vgacon_scrollback_cur->cur = vgacon_scrollback_cur->cnt;
	}
}