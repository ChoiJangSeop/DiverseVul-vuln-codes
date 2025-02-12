PredictorDecodeRow(TIFF* tif, uint8* op0, tmsize_t occ0, uint16 s)
{
	TIFFPredictorState *sp = PredictorState(tif);

	assert(sp != NULL);
	assert(sp->decoderow != NULL);
	assert(sp->decodepfunc != NULL);  

	if ((*sp->decoderow)(tif, op0, occ0, s)) {
		(*sp->decodepfunc)(tif, op0, occ0);
		return 1;
	} else
		return 0;
}