PixarLogClose(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;

	/* In a really sneaky (and really incorrect, and untruthful, and
	 * troublesome, and error-prone) maneuver that completely goes against
	 * the spirit of TIFF, and breaks TIFF, on close, we covertly
	 * modify both bitspersample and sampleformat in the directory to
	 * indicate 8-bit linear.  This way, the decode "just works" even for
	 * readers that don't know about PixarLog, or how to set
	 * the PIXARLOGDATFMT pseudo-tag.
	 */
	td->td_bitspersample = 8;
	td->td_sampleformat = SAMPLEFORMAT_UINT;
}