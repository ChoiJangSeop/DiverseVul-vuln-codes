static int ras_getdatastd(jas_stream_t *in, ras_hdr_t *hdr, ras_cmap_t *cmap,
  jas_image_t *image)
{
	int pad;
	int nz;
	int z;
	int c;
	int y;
	int x;
	int v;
	int i;
	jas_matrix_t *data[3];

/* Note: This function does not properly handle images with a colormap. */
	/* Avoid compiler warnings about unused parameters. */
	cmap = 0;

	for (i = 0; i < jas_image_numcmpts(image); ++i) {
		data[i] = jas_matrix_create(1, jas_image_width(image));
		assert(data[i]);
	}

	pad = RAS_ROWSIZE(hdr) - (hdr->width * hdr->depth + 7) / 8;

	for (y = 0; y < hdr->height; y++) {
		nz = 0;
		z = 0;
		for (x = 0; x < hdr->width; x++) {
			while (nz < hdr->depth) {
				if ((c = jas_stream_getc(in)) == EOF) {
					return -1;
				}
				z = (z << 8) | c;
				nz += 8;
			}

			v = (z >> (nz - hdr->depth)) & RAS_ONES(hdr->depth);
			z &= RAS_ONES(nz - hdr->depth);
			nz -= hdr->depth;

			if (jas_image_numcmpts(image) == 3) {
				jas_matrix_setv(data[0], x, (RAS_GETRED(v)));
				jas_matrix_setv(data[1], x, (RAS_GETGREEN(v)));
				jas_matrix_setv(data[2], x, (RAS_GETBLUE(v)));
			} else {
				jas_matrix_setv(data[0], x, (v));
			}
		}
		if (pad) {
			if ((c = jas_stream_getc(in)) == EOF) {
				return -1;
			}
		}
		for (i = 0; i < jas_image_numcmpts(image); ++i) {
			if (jas_image_writecmpt(image, i, 0, y, hdr->width, 1,
			  data[i])) {
				return -1;
			}
		}
	}

	for (i = 0; i < jas_image_numcmpts(image); ++i) {
		jas_matrix_destroy(data[i]);
	}

	return 0;
}