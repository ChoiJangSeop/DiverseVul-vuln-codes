int sbusfb_ioctl_helper(unsigned long cmd, unsigned long arg,
			struct fb_info *info,
			int type, int fb_depth, unsigned long fb_size)
{
	switch(cmd) {
	case FBIOGTYPE: {
		struct fbtype __user *f = (struct fbtype __user *) arg;

		if (put_user(type, &f->fb_type) ||
		    __put_user(info->var.yres, &f->fb_height) ||
		    __put_user(info->var.xres, &f->fb_width) ||
		    __put_user(fb_depth, &f->fb_depth) ||
		    __put_user(0, &f->fb_cmsize) ||
		    __put_user(fb_size, &f->fb_cmsize))
			return -EFAULT;
		return 0;
	}
	case FBIOPUTCMAP_SPARC: {
		struct fbcmap __user *c = (struct fbcmap __user *) arg;
		struct fb_cmap cmap;
		u16 red, green, blue;
		u8 red8, green8, blue8;
		unsigned char __user *ured;
		unsigned char __user *ugreen;
		unsigned char __user *ublue;
		int index, count, i;

		if (get_user(index, &c->index) ||
		    __get_user(count, &c->count) ||
		    __get_user(ured, &c->red) ||
		    __get_user(ugreen, &c->green) ||
		    __get_user(ublue, &c->blue))
			return -EFAULT;

		cmap.len = 1;
		cmap.red = &red;
		cmap.green = &green;
		cmap.blue = &blue;
		cmap.transp = NULL;
		for (i = 0; i < count; i++) {
			int err;

			if (get_user(red8, &ured[i]) ||
			    get_user(green8, &ugreen[i]) ||
			    get_user(blue8, &ublue[i]))
				return -EFAULT;

			red = red8 << 8;
			green = green8 << 8;
			blue = blue8 << 8;

			cmap.start = index + i;
			err = fb_set_cmap(&cmap, info);
			if (err)
				return err;
		}
		return 0;
	}
	case FBIOGETCMAP_SPARC: {
		struct fbcmap __user *c = (struct fbcmap __user *) arg;
		unsigned char __user *ured;
		unsigned char __user *ugreen;
		unsigned char __user *ublue;
		struct fb_cmap *cmap = &info->cmap;
		int index, count, i;
		u8 red, green, blue;

		if (get_user(index, &c->index) ||
		    __get_user(count, &c->count) ||
		    __get_user(ured, &c->red) ||
		    __get_user(ugreen, &c->green) ||
		    __get_user(ublue, &c->blue))
			return -EFAULT;

		if (index + count > cmap->len)
			return -EINVAL;

		for (i = 0; i < count; i++) {
			red = cmap->red[index + i] >> 8;
			green = cmap->green[index + i] >> 8;
			blue = cmap->blue[index + i] >> 8;
			if (put_user(red, &ured[i]) ||
			    put_user(green, &ugreen[i]) ||
			    put_user(blue, &ublue[i]))
				return -EFAULT;
		}
		return 0;
	}
	default:
		return -EINVAL;
	}
}