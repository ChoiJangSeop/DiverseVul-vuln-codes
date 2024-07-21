static void vgacon_scrollback_startup(void)
{
	vgacon_scrollback_cur = &vgacon_scrollbacks[0];
	vgacon_scrollback_init(0);
}