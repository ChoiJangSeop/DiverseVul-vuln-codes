PredictorDecodeTile(TIFF* tif, uint8* op0, tmsize_t occ0, uint16 s)
{
	TIFFPredictorState *sp = PredictorState(tif);

	assert(sp != NULL);
	assert(sp->decodetile != NULL);

	if ((*sp->decodetile)(tif, op0, occ0, s)) {
		tmsize_t rowsize = sp->rowsize;
		assert(rowsize > 0);
		assert((occ0%rowsize)==0);
		assert(sp->decodepfunc != NULL);
		while (occ0 > 0) {
			(*sp->decodepfunc)(tif, op0, rowsize);
			occ0 -= rowsize;
			op0 += rowsize;
		}
		return 1;
	} else
		return 0;
}