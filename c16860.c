TIFFReadBufferSetup(TIFF* tif, void* bp, tmsize_t size)
{
	static const char module[] = "TIFFReadBufferSetup";

	assert((tif->tif_flags&TIFF_NOREADRAW)==0);
	tif->tif_flags &= ~TIFF_BUFFERMMAP;

	if (tif->tif_rawdata) {
		if (tif->tif_flags & TIFF_MYBUFFER)
			_TIFFfree(tif->tif_rawdata);
		tif->tif_rawdata = NULL;
		tif->tif_rawdatasize = 0;
	}
	if (bp) {
		tif->tif_rawdatasize = size;
		tif->tif_rawdata = (uint8*) bp;
		tif->tif_flags &= ~TIFF_MYBUFFER;
	} else {
		tif->tif_rawdatasize = (tmsize_t)TIFFroundup_64((uint64)size, 1024);
		if (tif->tif_rawdatasize==0) {
		    TIFFErrorExt(tif->tif_clientdata, module,
				 "Invalid buffer size");
		    return (0);
		}
		tif->tif_rawdata = (uint8*) _TIFFmalloc(tif->tif_rawdatasize);
		tif->tif_flags |= TIFF_MYBUFFER;
	}
	if (tif->tif_rawdata == NULL) {
		TIFFErrorExt(tif->tif_clientdata, module,
		    "No space for data buffer at scanline %lu",
		    (unsigned long) tif->tif_row);
		tif->tif_rawdatasize = 0;
		return (0);
	}
	return (1);
}