LogLuvDecode24(TIFF* tif, uint8* op, tmsize_t occ, uint16 s)
{
	static const char module[] = "LogLuvDecode24";
	LogLuvState* sp = DecoderState(tif);
	tmsize_t cc;
	tmsize_t i;
	tmsize_t npixels;
	unsigned char* bp;
	uint32* tp;

	assert(s == 0);
	assert(sp != NULL);

	npixels = occ / sp->pixel_size;

	if (sp->user_datafmt == SGILOGDATAFMT_RAW)
		tp = (uint32 *)op;
	else {
		assert(sp->tbuflen >= npixels);
		tp = (uint32 *) sp->tbuf;
	}
	/* copy to array of uint32 */
	bp = (unsigned char*) tif->tif_rawcp;
	cc = tif->tif_rawcc;
	for (i = 0; i < npixels && cc > 0; i++) {
		tp[i] = bp[0] << 16 | bp[1] << 8 | bp[2];
		bp += 3;
		cc -= 3;
	}
	tif->tif_rawcp = (uint8*) bp;
	tif->tif_rawcc = cc;
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
		return (0);
	}
	(*sp->tfunc)(sp, op, npixels);
	return (1);
}