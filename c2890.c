static jas_image_cmpt_t *jas_image_cmpt_create(uint_fast32_t tlx, uint_fast32_t tly,
  uint_fast32_t hstep, uint_fast32_t vstep, uint_fast32_t width, uint_fast32_t
  height, uint_fast16_t depth, bool sgnd, uint_fast32_t inmem)
{
	jas_image_cmpt_t *cmpt;
	long size;

	if (!(cmpt = jas_malloc(sizeof(jas_image_cmpt_t)))) {
		return 0;
	}

	cmpt->type_ = JAS_IMAGE_CT_UNKNOWN;
	cmpt->tlx_ = tlx;
	cmpt->tly_ = tly;
	cmpt->hstep_ = hstep;
	cmpt->vstep_ = vstep;
	cmpt->width_ = width;
	cmpt->height_ = height;
	cmpt->prec_ = depth;
	cmpt->sgnd_ = sgnd;
	cmpt->stream_ = 0;
	cmpt->cps_ = (depth + 7) / 8;

	size = cmpt->width_ * cmpt->height_ * cmpt->cps_;
	cmpt->stream_ = (inmem) ? jas_stream_memopen(0, size) : jas_stream_tmpfile();
	if (!cmpt->stream_) {
		jas_image_cmpt_destroy(cmpt);
		return 0;
	}

	/* Zero the component data.  This isn't necessary, but it is
	convenient for debugging purposes. */
	if (jas_stream_seek(cmpt->stream_, size - 1, SEEK_SET) < 0 ||
	  jas_stream_putc(cmpt->stream_, 0) == EOF ||
	  jas_stream_seek(cmpt->stream_, 0, SEEK_SET) < 0) {
		jas_image_cmpt_destroy(cmpt);
		return 0;
	}

	return cmpt;
}