static bmp_info_t *bmp_getinfo(jas_stream_t *in)
{
	bmp_info_t *info;
	int i;
	bmp_palent_t *palent;

	if (!(info = bmp_info_create())) {
		return 0;
	}

	if (bmp_getint32(in, &info->len) || info->len != 40 ||
	  bmp_getint32(in, &info->width) || bmp_getint32(in, &info->height) ||
	  bmp_getint16(in, &info->numplanes) ||
	  bmp_getint16(in, &info->depth) || bmp_getint32(in, &info->enctype) ||
	  bmp_getint32(in, &info->siz) ||
	  bmp_getint32(in, &info->hres) || bmp_getint32(in, &info->vres) ||
	  bmp_getint32(in, &info->numcolors) ||
	  bmp_getint32(in, &info->mincolors)) {
		bmp_info_destroy(info);
		return 0;
	}

	if (info->height < 0) {
		info->topdown = 1;
		info->height = -info->height;
	} else {
		info->topdown = 0;
	}

	if (info->width <= 0 || info->height <= 0 || info->numplanes <= 0 ||
	  info->depth <= 0  || info->numcolors < 0 || info->mincolors < 0) {
		bmp_info_destroy(info);
		return 0;
	}

	if (info->enctype != BMP_ENC_RGB) {
		jas_eprintf("unsupported BMP encoding\n");
		bmp_info_destroy(info);
		return 0;
	}

	if (info->numcolors > 0) {
		if (!(info->palents = jas_malloc(info->numcolors *
		  sizeof(bmp_palent_t)))) {
			bmp_info_destroy(info);
			return 0;
		}
	} else {
		info->palents = 0;
	}

	for (i = 0; i < info->numcolors; ++i) {
		palent = &info->palents[i];
		if ((palent->blu = jas_stream_getc(in)) == EOF ||
		  (palent->grn = jas_stream_getc(in)) == EOF ||
		  (palent->red = jas_stream_getc(in)) == EOF ||
		  (palent->res = jas_stream_getc(in)) == EOF) {
			bmp_info_destroy(info);
			return 0;
		}
	}

	return info;
}