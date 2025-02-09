PredictorEncodeRow(TIFF* tif, uint8* bp, tmsize_t cc, uint16 s)
{
	TIFFPredictorState *sp = PredictorState(tif);

	assert(sp != NULL);
	assert(sp->encodepfunc != NULL);
	assert(sp->encoderow != NULL);

	/* XXX horizontal differencing alters user's data XXX */
	(*sp->encodepfunc)(tif, bp, cc);
	return (*sp->encoderow)(tif, bp, cc, s);
}