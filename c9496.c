static void vgacon_scrollback_reset(int vc_num, size_t reset_size)
{
	struct vgacon_scrollback_info *scrollback = &vgacon_scrollbacks[vc_num];

	if (scrollback->data && reset_size > 0)
		memset(scrollback->data, 0, reset_size);

	scrollback->cnt  = 0;
	scrollback->tail = 0;
	scrollback->cur  = 0;
}