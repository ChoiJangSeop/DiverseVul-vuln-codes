static int jas_icclut8_input(jas_iccattrval_t *attrval, jas_stream_t *in,
  int cnt)
{
	int i;
	int j;
	int clutsize;
	jas_icclut8_t *lut8 = &attrval->data.lut8;
	lut8->clut = 0;
	lut8->intabs = 0;
	lut8->intabsbuf = 0;
	lut8->outtabs = 0;
	lut8->outtabsbuf = 0;
	if (jas_iccgetuint8(in, &lut8->numinchans) ||
	  jas_iccgetuint8(in, &lut8->numoutchans) ||
	  jas_iccgetuint8(in, &lut8->clutlen) ||
	  jas_stream_getc(in) == EOF)
		goto error;
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {
			if (jas_iccgetsint32(in, &lut8->e[i][j]))
				goto error;
		}
	}
	if (jas_iccgetuint16(in, &lut8->numintabents) ||
	  jas_iccgetuint16(in, &lut8->numouttabents))
		goto error;
	clutsize = jas_iccpowi(lut8->clutlen, lut8->numinchans) * lut8->numoutchans;
	if (!(lut8->clut = jas_malloc(clutsize * sizeof(jas_iccuint8_t))) ||
	  !(lut8->intabsbuf = jas_malloc(lut8->numinchans *
	  lut8->numintabents * sizeof(jas_iccuint8_t))) ||
	  !(lut8->intabs = jas_malloc(lut8->numinchans *
	  sizeof(jas_iccuint8_t *))))
		goto error;
	for (i = 0; i < lut8->numinchans; ++i)
		lut8->intabs[i] = &lut8->intabsbuf[i * lut8->numintabents];
	if (!(lut8->outtabsbuf = jas_malloc(lut8->numoutchans *
	  lut8->numouttabents * sizeof(jas_iccuint8_t))) ||
	  !(lut8->outtabs = jas_malloc(lut8->numoutchans *
	  sizeof(jas_iccuint8_t *))))
		goto error;
	for (i = 0; i < lut8->numoutchans; ++i)
		lut8->outtabs[i] = &lut8->outtabsbuf[i * lut8->numouttabents];
	for (i = 0; i < lut8->numinchans; ++i) {
		for (j = 0; j < JAS_CAST(int, lut8->numintabents); ++j) {
			if (jas_iccgetuint8(in, &lut8->intabs[i][j]))
				goto error;
		}
	}
	for (i = 0; i < lut8->numoutchans; ++i) {
		for (j = 0; j < JAS_CAST(int, lut8->numouttabents); ++j) {
			if (jas_iccgetuint8(in, &lut8->outtabs[i][j]))
				goto error;
		}
	}
	for (i = 0; i < clutsize; ++i) {
		if (jas_iccgetuint8(in, &lut8->clut[i]))
			goto error;
	}
	if (JAS_CAST(int, 44 + lut8->numinchans * lut8->numintabents +
	  lut8->numoutchans * lut8->numouttabents +
	  jas_iccpowi(lut8->clutlen, lut8->numinchans) * lut8->numoutchans) !=
	  cnt)
		goto error;
	return 0;
error:
	jas_icclut8_destroy(attrval);
	return -1;
}