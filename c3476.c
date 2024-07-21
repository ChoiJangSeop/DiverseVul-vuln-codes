int jas_image_writecmpt(jas_image_t *image, int cmptno, jas_image_coord_t x, jas_image_coord_t y, jas_image_coord_t width,
  jas_image_coord_t height, jas_matrix_t *data)
{
	jas_image_cmpt_t *cmpt;
	jas_image_coord_t i;
	jas_image_coord_t j;
	jas_seqent_t *d;
	jas_seqent_t *dr;
	int drs;
	jas_seqent_t v;
	int k;
	int c;

	if (cmptno < 0 || cmptno >= image->numcmpts_) {
		return -1;
	}

	cmpt = image->cmpts_[cmptno];
	if (x >= cmpt->width_ || y >= cmpt->height_ ||
	  x + width > cmpt->width_ ||
	  y + height > cmpt->height_) {
		return -1;
	}

	if (jas_matrix_numrows(data) != height || jas_matrix_numcols(data) != width) {
		return -1;
	}

	dr = jas_matrix_getref(data, 0, 0);
	drs = jas_matrix_rowstep(data);
	for (i = 0; i < height; ++i, dr += drs) {
		d = dr;
		if (jas_stream_seek(cmpt->stream_, (cmpt->width_ * (y + i) + x)
		  * cmpt->cps_, SEEK_SET) < 0) {
			return -1;
		}
		for (j = width; j > 0; --j, ++d) {
			v = inttobits(*d, cmpt->prec_, cmpt->sgnd_);
			for (k = cmpt->cps_; k > 0; --k) {
				c = (v >> (8 * (cmpt->cps_ - 1))) & 0xff;
				if (jas_stream_putc(cmpt->stream_,
				  (unsigned char) c) == EOF) {
					return -1;
				}
				v <<= 8;
			}
		}
	}

	return 0;
}