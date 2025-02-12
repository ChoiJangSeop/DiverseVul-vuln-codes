horAcc16(TIFF* tif, uint8* cp0, tmsize_t cc)
{
	tmsize_t stride = PredictorState(tif)->stride;
	uint16* wp = (uint16*) cp0;
	tmsize_t wc = cc / 2;

	assert((cc%(2*stride))==0);

	if (wc > stride) {
		wc -= stride;
		do {
			REPEAT4(stride, wp[stride] = (uint16)(((unsigned int)wp[stride] + (unsigned int)wp[0]) & 0xffff); wp++)
			wc -= stride;
		} while (wc > 0);
	}
}