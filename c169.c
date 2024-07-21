static int jas_cmshapmatlut_invert(jas_cmshapmatlut_t *invlut,
  jas_cmshapmatlut_t *lut, int n)
{
	int i;
	int j;
	int k;
	jas_cmreal_t ax;
	jas_cmreal_t ay;
	jas_cmreal_t bx;
	jas_cmreal_t by;
	jas_cmreal_t sx;
	jas_cmreal_t sy;
	assert(n >= 2);
	if (invlut->data) {
		jas_free(invlut->data);
		invlut->data = 0;
	}
	/* The sample values should be nondecreasing. */
	for (i = 1; i < lut->size; ++i) {
		if (lut->data[i - 1] > lut->data[i]) {
			assert(0);
			return -1;
		}
	}
	if (!(invlut->data = jas_malloc(n * sizeof(jas_cmreal_t))))
		return -1;
	invlut->size = n;
	for (i = 0; i < invlut->size; ++i) {
		sy = ((double) i) / (invlut->size - 1);
		sx = 1.0;
		for (j = 0; j < lut->size; ++j) {
			ay = lut->data[j];
			if (sy == ay) {
				for (k = j + 1; k < lut->size; ++k) {
					by = lut->data[k];
					if (by != sy)
						break;
#if 0
assert(0);
#endif
				}
				if (k < lut->size) {
					--k;
					ax = ((double) j) / (lut->size - 1);
					bx = ((double) k) / (lut->size - 1);
					sx = (ax + bx) / 2.0;
				}
				break;
			}
			if (j < lut->size - 1) {
				by = lut->data[j + 1];
				if (sy > ay && sy < by) {
					ax = ((double) j) / (lut->size - 1);
					bx = ((double) j + 1) / (lut->size - 1);
					sx = ax +
					  (sy - ay) / (by - ay) * (bx - ax);
					break;
				}
			}
		}
		invlut->data[i] = sx;
	}
#if 0
for (i=0;i<lut->size;++i)
	jas_eprintf("lut[%d]=%f ", i, lut->data[i]);
for (i=0;i<invlut->size;++i)
	jas_eprintf("invlut[%d]=%f ", i, invlut->data[i]);
#endif
	return 0;
}