static void vgacon_scrolldelta(struct vc_data *c, int lines)
{
	vc_scrolldelta_helper(c, lines, vga_rolled_over, (void *)vga_vram_base,
			vga_vram_size);
	vga_set_mem_top(c);
}