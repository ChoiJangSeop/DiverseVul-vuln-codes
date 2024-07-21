compat_fontx_ioctl(int cmd, struct compat_consolefontdesc __user *user_cfd,
			 int perm, struct console_font_op *op)
{
	struct compat_consolefontdesc cfdarg;
	int i;

	if (copy_from_user(&cfdarg, user_cfd, sizeof(struct compat_consolefontdesc)))
		return -EFAULT;

	switch (cmd) {
	case PIO_FONTX:
		if (!perm)
			return -EPERM;
		op->op = KD_FONT_OP_SET;
		op->flags = KD_FONT_FLAG_OLD;
		op->width = 8;
		op->height = cfdarg.charheight;
		op->charcount = cfdarg.charcount;
		op->data = compat_ptr(cfdarg.chardata);
		return con_font_op(vc_cons[fg_console].d, op);
	case GIO_FONTX:
		op->op = KD_FONT_OP_GET;
		op->flags = KD_FONT_FLAG_OLD;
		op->width = 8;
		op->height = cfdarg.charheight;
		op->charcount = cfdarg.charcount;
		op->data = compat_ptr(cfdarg.chardata);
		i = con_font_op(vc_cons[fg_console].d, op);
		if (i)
			return i;
		cfdarg.charheight = op->height;
		cfdarg.charcount = op->charcount;
		if (copy_to_user(user_cfd, &cfdarg, sizeof(struct compat_consolefontdesc)))
			return -EFAULT;
		return 0;
	}
	return -EINVAL;
}