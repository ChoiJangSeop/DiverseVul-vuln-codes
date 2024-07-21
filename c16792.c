fpAcc(TIFF* tif, uint8* cp0, tmsize_t cc)
{
	tmsize_t stride = PredictorState(tif)->stride;
	uint32 bps = tif->tif_dir.td_bitspersample / 8;
	tmsize_t wc = cc / bps;
	tmsize_t count = cc;
	uint8 *cp = (uint8 *) cp0;
	uint8 *tmp = (uint8 *)_TIFFmalloc(cc);

	assert((cc%(bps*stride))==0);

	if (!tmp)
		return;

	while (count > stride) {
		REPEAT4(stride, cp[stride] =
                        (unsigned char) ((cp[stride] + cp[0]) & 0xff); cp++)
		count -= stride;
	}

	_TIFFmemcpy(tmp, cp0, cc);
	cp = (uint8 *) cp0;
	for (count = 0; count < wc; count++) {
		uint32 byte;
		for (byte = 0; byte < bps; byte++) {
			#if WORDS_BIGENDIAN
			cp[bps * count + byte] = tmp[byte * wc + count];
			#else
			cp[bps * count + byte] =
				tmp[(bps - byte - 1) * wc + count];
			#endif
		}
	}
	_TIFFfree(tmp);
}