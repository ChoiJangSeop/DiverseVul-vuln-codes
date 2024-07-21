static void vgacon_scrollback_init(int vc_num)
{
	int pitch = vga_video_num_columns * 2;
	size_t size = CONFIG_VGACON_SOFT_SCROLLBACK_SIZE * 1024;
	int rows = size / pitch;
	void *data;

	data = kmalloc_array(CONFIG_VGACON_SOFT_SCROLLBACK_SIZE, 1024,
			     GFP_NOWAIT);

	vgacon_scrollbacks[vc_num].data = data;
	vgacon_scrollback_cur = &vgacon_scrollbacks[vc_num];

	vgacon_scrollback_cur->rows = rows - 1;
	vgacon_scrollback_cur->size = rows * pitch;

	vgacon_scrollback_reset(vc_num, size);
}