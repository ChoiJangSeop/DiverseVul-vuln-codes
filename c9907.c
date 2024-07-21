int ReadStrip(TIFF* tiff, UINT32 row, UINT32* buffer) {
    uint16 photometric = 0; // init to not PHOTOMETRIC_YCBCR
    TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric);

    // To avoid dealing with YCbCr subsampling, let libtiff handle it
    if (photometric == PHOTOMETRIC_YCBCR) {
        TIFFRGBAImage img;
        char emsg[1024] = "";
        UINT32 rows_per_strip, rows_to_read;
        int ok;


        TIFFGetFieldDefaulted(tiff, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);
        if ((row % rows_per_strip) != 0) {
            TRACE(("Row passed to ReadStrip() must be first in a strip."));
            return -1;
        }

        if (TIFFRGBAImageOK(tiff, emsg) && TIFFRGBAImageBegin(&img, tiff, 0, emsg)) {
            TRACE(("Initialized RGBAImage\n"));

            img.req_orientation = ORIENTATION_TOPLEFT;
            img.row_offset = row;
            img.col_offset = 0;

            rows_to_read = min(rows_per_strip, img.height - row);

            TRACE(("rows to read: %d\n", rows_to_read));
            ok = TIFFRGBAImageGet(&img, buffer, img.width, rows_to_read);

            TIFFRGBAImageEnd(&img);
        } else {
            ok = 0;
        }

        if (ok == 0) {
            TRACE(("Decode Error, row %d; msg: %s\n", row, emsg));
            return -1;
        }

        return 0;
    }

    if (TIFFReadEncodedStrip(tiff, TIFFComputeStrip(tiff, row, 0), (tdata_t)buffer, -1) == -1) {
        TRACE(("Decode Error, strip %d\n", TIFFComputeStrip(tiff, row, 0)));
        return -1;
    }

    return 0;
}