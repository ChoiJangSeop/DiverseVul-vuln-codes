static int vt_io_ioctl(struct vc_data *vc, unsigned int cmd, void __user *up,
		bool perm)
{
	struct console_font_op op;	/* used in multiple places here */

	switch (cmd) {
	case PIO_FONT:
		if (!perm)
			return -EPERM;
		op.op = KD_FONT_OP_SET;
		op.flags = KD_FONT_FLAG_OLD | KD_FONT_FLAG_DONT_RECALC;	/* Compatibility */
		op.width = 8;
		op.height = 0;
		op.charcount = 256;
		op.data = up;
		return con_font_op(vc_cons[fg_console].d, &op);

	case GIO_FONT:
		op.op = KD_FONT_OP_GET;
		op.flags = KD_FONT_FLAG_OLD;
		op.width = 8;
		op.height = 32;
		op.charcount = 256;
		op.data = up;
		return con_font_op(vc_cons[fg_console].d, &op);

	case PIO_CMAP:
                if (!perm)
			return -EPERM;
		return con_set_cmap(up);

	case GIO_CMAP:
                return con_get_cmap(up);

	case PIO_FONTX:
		if (!perm)
			return -EPERM;

		fallthrough;
	case GIO_FONTX:
		return do_fontx_ioctl(cmd, up, &op);

	case PIO_FONTRESET:
		if (!perm)
			return -EPERM;

		return vt_io_fontreset(&op);

	case PIO_SCRNMAP:
		if (!perm)
			return -EPERM;
		return con_set_trans_old(up);

	case GIO_SCRNMAP:
		return con_get_trans_old(up);

	case PIO_UNISCRNMAP:
		if (!perm)
			return -EPERM;
		return con_set_trans_new(up);

	case GIO_UNISCRNMAP:
		return con_get_trans_new(up);

	case PIO_UNIMAPCLR:
		if (!perm)
			return -EPERM;
		con_clear_unimap(vc);
		break;

	case PIO_UNIMAP:
	case GIO_UNIMAP:
		return do_unimap_ioctl(cmd, up, perm, vc);

	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}