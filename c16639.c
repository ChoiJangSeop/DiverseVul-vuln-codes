LogLuvDecode32(TIFF* tif, uint8* op, tmsize_t occ, uint16 s)
{
	static const char module[] = "LogLuvDecode32";
	LogLuvState* sp;
	int shft;
	tmsize_t i;
	tmsize_t npixels;
	unsigned char* bp;
	uint32* tp;
	uint32 b;
	tmsize_t cc;
	int rc;

	assert(s == 0);
	sp = DecoderState(tif);
	assert(sp != NULL);

	npixels = occ / sp->pixel_size;

	if (sp->user_datafmt == SGILOGDATAFMT_RAW)
		tp = (uint32*) op;
	else {
		assert(sp->tbuflen >= npixels);
		tp = (uint32*) sp->tbuf;
	}
	_TIFFmemset((void*) tp, 0, npixels*sizeof (tp[0]));

	bp = (unsigned char*) tif->tif_rawcp;
	cc = tif->tif_rawcc;
	/* get each byte string */
	for (shft = 4*8; (shft -= 8) >= 0; ) {
		for (i = 0; i < npixels && cc > 0; )
			if (*bp >= 128) {		/* run */
				rc = *bp++ + (2-128);
				b = (uint32)*bp++ << shft;
				cc -= 2;                /* TODO: potential input buffer overrun when decoding corrupt or truncated data */
				while (rc-- && i < npixels)
					tp[i++] |= b;
			} else {			/* non-run */
				rc = *bp++;		/* nul is noop */
				while (--cc && rc-- && i < npixels)
					tp[i++] |= (uint32)*bp++ << shft;
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