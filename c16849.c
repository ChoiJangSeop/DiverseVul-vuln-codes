TIFFNumberOfStrips(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	uint32 nstrips;

    /* If the value was already computed and store in td_nstrips, then return it,
       since ChopUpSingleUncompressedStrip might have altered and resized the
       since the td_stripbytecount and td_stripoffset arrays to the new value
       after the initial affectation of td_nstrips = TIFFNumberOfStrips() in
       tif_dirread.c ~line 3612.
       See http://bugzilla.maptools.org/show_bug.cgi?id=2587 */
    if( td->td_nstrips )
        return td->td_nstrips;

	nstrips = (td->td_rowsperstrip == (uint32) -1 ? 1 :
	     TIFFhowmany_32(td->td_imagelength, td->td_rowsperstrip));
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
		nstrips = _TIFFMultiply32(tif, nstrips, (uint32)td->td_samplesperpixel,
		    "TIFFNumberOfStrips");
	return (nstrips);
}