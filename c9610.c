static int vt_io_fontreset(struct console_font_op *op)
{
	int ret;

	if (__is_defined(BROKEN_GRAPHICS_PROGRAMS)) {
		/*
		 * With BROKEN_GRAPHICS_PROGRAMS defined, the default font is
		 * not saved.
		 */
		return -ENOSYS;
	}

	op->op = KD_FONT_OP_SET_DEFAULT;
	op->data = NULL;
	ret = con_font_op(vc_cons[fg_console].d, op);
	if (ret)
		return ret;

	console_lock();
	con_set_default_unimap(vc_cons[fg_console].d);
	console_unlock();

	return 0;
}