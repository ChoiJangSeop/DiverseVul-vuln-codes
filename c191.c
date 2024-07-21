static int jas_icclut16_input(jas_iccattrval_t *attrval, jas_stream_t *in,
  int cnt)
{
	int i;
	int j;
	int clutsize;
	jas_icclut16_t *lut16 = &attrval->data.lut16;
	lut16->clut = 0;
	lut16->intabs = 0;
	lut16->intabsbuf = 0;
	lut16->outtabs = 0;
	lut16->outtabsbuf = 0;
	if (jas_iccgetuint8(in, &lut16->numinchans) ||
	  jas_iccgetuint8(in, &lut16->numoutchans) ||
	  jas_iccgetuint8(in, &lut16->clutlen) ||
	  jas_stream_getc(in) == EOF)
		goto error;
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			if (jas_iccgetsint32(in, &lut16->e[i][j]))
				goto error;
		}
	}
	if (jas_iccgetuint16(in, &lut16->numintabents) ||
	  jas_iccgetuint16(in, &lut16->numouttabents))
		goto error;
	clutsize = jas_iccpowi(lut16->clutlen, lut16->numinchans) * lut16->numoutchans;
	if (!(lut16->clut = jas_malloc(clutsize * sizeof(jas_iccuint16_t))) ||
	  !(lut16->intabsbuf = jas_malloc(lut16->numinchans *
	  lut16->numintabents * sizeof(jas_iccuint16_t))) ||
	  !(lut16->intabs = jas_malloc(lut16->numinchans *
	  sizeof(jas_iccuint16_t *))))
		goto error;
	for (i = 0; i < lut16->numinchans; ++i)
		lut16->intabs[i] = &lut16->intabsbuf[i * lut16->numintabents];
	if (!(lut16->outtabsbuf = jas_malloc(lut16->numoutchans *
	  lut16->numouttabents * sizeof(jas_iccuint16_t))) ||
	  !(lut16->outtabs = jas_malloc(lut16->numoutchans *
	  sizeof(jas_iccuint16_t *))))
		goto error;
	for (i = 0; i < lut16->numoutchans; ++i)
		lut16->outtabs[i] = &lut16->outtabsbuf[i * lut16->numouttabents];
	for (i = 0; i < lut16->numinchans; ++i) {
		for (j = 0; j < JAS_CAST(int, lut16->numintabents); ++j) {
			if (jas_iccgetuint16(in, &lut16->intabs[i][j]))
				goto error;
		}
	}
	for (i = 0; i < lut16->numoutchans; ++i) {
		for (j = 0; j < JAS_CAST(int, lut16->numouttabents); ++j) {
			if (jas_iccgetuint16(in, &lut16->outtabs[i][j]))
				goto error;
		}
	}
	for (i = 0; i < clutsize; ++i) {
		if (jas_iccgetuint16(in, &lut16->clut[i]))
			goto error;
	}
	if (JAS_CAST(int, 44 + 2 * (lut16->numinchans * lut16->numintabents +
          lut16->numoutchans * lut16->numouttabents +
          jas_iccpowi(lut16->clutlen, lut16->numinchans) *
	  lut16->numoutchans)) != cnt)
		goto error;
	return 0;
error:
	jas_icclut16_destroy(attrval);
	return -1;
}