long vt_compat_ioctl(struct tty_struct *tty,
	     unsigned int cmd, unsigned long arg)
{
	struct vc_data *vc = tty->driver_data;
	struct console_font_op op;	/* used in multiple places here */
	void __user *up = compat_ptr(arg);
	int perm;

	/*
	 * To have permissions to do most of the vt ioctls, we either have
	 * to be the owner of the tty, or have CAP_SYS_TTY_CONFIG.
	 */
	perm = 0;
	if (current->signal->tty == tty || capable(CAP_SYS_TTY_CONFIG))
		perm = 1;

	switch (cmd) {
	/*
	 * these need special handlers for incompatible data structures
	 */
	case PIO_FONTX:
	case GIO_FONTX:
		return compat_fontx_ioctl(cmd, up, perm, &op);

	case KDFONTOP:
		return compat_kdfontop_ioctl(up, perm, &op, vc);

	case PIO_UNIMAP:
	case GIO_UNIMAP:
		return compat_unimap_ioctl(cmd, up, perm, vc);

	/*
	 * all these treat 'arg' as an integer
	 */
	case KIOCSOUND:
	case KDMKTONE:
#ifdef CONFIG_X86
	case KDADDIO:
	case KDDELIO:
#endif
	case KDSETMODE:
	case KDMAPDISP:
	case KDUNMAPDISP:
	case KDSKBMODE:
	case KDSKBMETA:
	case KDSKBLED:
	case KDSETLED:
	case KDSIGACCEPT:
	case VT_ACTIVATE:
	case VT_WAITACTIVE:
	case VT_RELDISP:
	case VT_DISALLOCATE:
	case VT_RESIZE:
	case VT_RESIZEX:
		return vt_ioctl(tty, cmd, arg);

	/*
	 * the rest has a compatible data structure behind arg,
	 * but we have to convert it to a proper 64 bit pointer.
	 */
	default:
		return vt_ioctl(tty, cmd, (unsigned long)up);
	}
}