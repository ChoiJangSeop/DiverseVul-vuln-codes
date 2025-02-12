PixarLogSetupDecode(TIFF* tif)
{
	static const char module[] = "PixarLogSetupDecode";
	TIFFDirectory *td = &tif->tif_dir;
	PixarLogState* sp = DecoderState(tif);
	tmsize_t tbuf_size;

	assert(sp != NULL);

	/* Make sure no byte swapping happens on the data
	 * after decompression. */
	tif->tif_postdecode = _TIFFNoPostDecode;  

	/* for some reason, we can't do this in TIFFInitPixarLog */

	sp->stride = (td->td_planarconfig == PLANARCONFIG_CONTIG ?
	    td->td_samplesperpixel : 1);
	tbuf_size = multiply_ms(multiply_ms(multiply_ms(sp->stride, td->td_imagewidth),
				      td->td_rowsperstrip), sizeof(uint16));
	/* add one more stride in case input ends mid-stride */
	tbuf_size = add_ms(tbuf_size, sizeof(uint16) * sp->stride);
	if (tbuf_size == 0)
		return (0);   /* TODO: this is an error return without error report through TIFFErrorExt */
	sp->tbuf = (uint16 *) _TIFFmalloc(tbuf_size);
	if (sp->tbuf == NULL)
		return (0);
	if (sp->user_datafmt == PIXARLOGDATAFMT_UNKNOWN)
		sp->user_datafmt = PixarLogGuessDataFmt(td);
	if (sp->user_datafmt == PIXARLOGDATAFMT_UNKNOWN) {
		TIFFErrorExt(tif->tif_clientdata, module,
			"PixarLog compression can't handle bits depth/data format combination (depth: %d)", 
			td->td_bitspersample);
		return (0);
	}

	if (inflateInit(&sp->stream) != Z_OK) {
		TIFFErrorExt(tif->tif_clientdata, module, "%s", sp->stream.msg ? sp->stream.msg : "(null)");
		return (0);
	} else {
		sp->state |= PLSTATE_INIT;
		return (1);
	}
}