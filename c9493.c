static void vgacon_scrollback_switch(int vc_num)
{
	if (!scrollback_persistent)
		vc_num = 0;

	if (!vgacon_scrollbacks[vc_num].data) {
		vgacon_scrollback_init(vc_num);
	} else {
		if (scrollback_persistent) {
			vgacon_scrollback_cur = &vgacon_scrollbacks[vc_num];
		} else {
			size_t size = CONFIG_VGACON_SOFT_SCROLLBACK_SIZE * 1024;

			vgacon_scrollback_reset(vc_num, size);
		}
	}
}