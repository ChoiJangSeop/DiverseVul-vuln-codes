static void vgacon_restore_screen(struct vc_data *c)
{
	if (c->vc_origin != c->vc_visible_origin)
		vgacon_scrolldelta(c, 0);
}