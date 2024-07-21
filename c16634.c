LogL16Decode(TIFF* tif, uint8* op, tmsize_t occ, uint16 s)
{
	static const char module[] = "LogL16Decode";
	LogLuvState* sp = DecoderState(tif);
	int shft;
	tmsize_t i;
	tmsize_t npixels;
	unsigned char* bp;
	int16* tp;
	int16 b;
	tmsize_t cc;
	int rc;

	assert(s == 0);
	assert(sp != NULL);

	npixels = occ / sp->pixel_size;

	if (sp->user_datafmt == SGILOGDATAFMT_16BIT)
		tp = (int16*) op;
	else {
		assert(sp->tbuflen >= npixels);
		tp = (int16*) sp->tbuf;
	}
	_TIFFmemset((void*) tp, 0, npixels*sizeof (tp[0]));

	bp = (unsigned char*) tif->tif_rawcp;
	cc = tif->tif_rawcc;
	/* get each byte string */
	for (shft = 2*8; (shft -= 8) >= 0; ) {
		for (i = 0; i < npixels && cc > 0; )
			if (*bp >= 128) {		/* run */
				rc = *bp++ + (2-128);   /* TODO: potential input buffer overrun when decoding corrupt or truncated data */
				b = (int16)(*bp++ << shft);
				cc -= 2;
				while (rc-- && i < npixels)
					tp[i++] |= b;
			} else {			/* non-run */
				rc = *bp++;		/* nul is noop */
				while (--cc && rc-- && i < npixels)
					tp[i++] |= (int16)*bp++ << shft;
			}
		if (i != npixels) {
#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
			TIFFErrorExt(tif->tif_clientdata, module,
			    "Not enough data at row %lu (short %I64d pixels)",
				     (unsigned long) tif->tif_row,
				     (unsigned __int64) (npixels - i));
#else
			TIFFErrorExt(tif->tif_clientdata, module,
			    "Not enough data at row %lu (short %llu pixels)",
				     (unsigned long) tif->tif_row,
				     (unsigned long long) (npixels - i));
#endif
			tif->tif_rawcp = (uint8*) bp;
			tif->tif_rawcc = cc;
			return (0);
		}
	}
	(*sp->tfunc)(sp, op, npixels);
	tif->tif_rawcp = (uint8*) bp;
	tif->tif_rawcc = cc;
	return (1);
}