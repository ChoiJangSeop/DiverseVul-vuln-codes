horDiff32(TIFF* tif, uint8* cp0, tmsize_t cc)
{
	TIFFPredictorState* sp = PredictorState(tif);
	tmsize_t stride = sp->stride;
	uint32 *wp = (uint32*) cp0;
	tmsize_t wc = cc/4;

	assert((cc%(4*stride))==0);

	if (wc > stride) {
		wc -= stride;
		wp += wc - 1;
		do {
			REPEAT4(stride, wp[stride] -= wp[0]; wp--)
			wc -= stride;
		} while (wc > 0);
	}
}