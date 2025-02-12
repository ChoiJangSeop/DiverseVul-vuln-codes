static int jas_cmshapmatlut_set(jas_cmshapmatlut_t *lut, jas_icccurv_t *curv)
{
	jas_cmreal_t gamma;
	int i;
	gamma = 0;
	jas_cmshapmatlut_cleanup(lut);
	if (curv->numents == 0) {
		lut->size = 2;
		if (!(lut->data = jas_malloc(lut->size * sizeof(jas_cmreal_t))))
			goto error;
		lut->data[0] = 0.0;
		lut->data[1] = 1.0;
	} else if (curv->numents == 1) {
		lut->size = 256;
		if (!(lut->data = jas_malloc(lut->size * sizeof(jas_cmreal_t))))
			goto error;
		gamma = curv->ents[0] / 256.0;
		for (i = 0; i < lut->size; ++i) {
			lut->data[i] = gammafn(i / (double) (lut->size - 1), gamma);
		}
	} else {
		lut->size = curv->numents;
		if (!(lut->data = jas_malloc(lut->size * sizeof(jas_cmreal_t))))
			goto error;
		for (i = 0; i < lut->size; ++i) {
			lut->data[i] = curv->ents[i] / 65535.0;
		}
	}
	return 0;
error:
	return -1;
}