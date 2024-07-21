static void vgacon_flush_scrollback(struct vc_data *c)
{
	size_t size = CONFIG_VGACON_SOFT_SCROLLBACK_SIZE * 1024;

	vgacon_scrollback_reset(c->vc_num, size);
}