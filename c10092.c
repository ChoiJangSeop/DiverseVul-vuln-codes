cvt_whole_image( TIFF *in, TIFF *out )

{
    uint32* raster;			/* retrieve RGBA image */
    uint32  width, height;		/* image width & height */
    uint32  row;
    size_t pixel_count;
        
    TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(in, TIFFTAG_IMAGELENGTH, &height);
    pixel_count = width * height;

    /* XXX: Check the integer overflow. */
    if (!width || !height || pixel_count / width != height) {
        TIFFError(TIFFFileName(in),
		  "Malformed input file; can't allocate buffer for raster of %lux%lu size",
		  (unsigned long)width, (unsigned long)height);
        return 0;
    }

    rowsperstrip = TIFFDefaultStripSize(out, rowsperstrip);
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip);

    raster = (uint32*)_TIFFCheckMalloc(in, pixel_count, sizeof(uint32), "raster buffer");
    if (raster == 0) {
        TIFFError(TIFFFileName(in), "Failed to allocate buffer (%lu elements of %lu each)",
		  (unsigned long)pixel_count, (unsigned long)sizeof(uint32));
        return (0);
    }

    /* Read the image in one chunk into an RGBA array */
    if (!TIFFReadRGBAImageOriented(in, width, height, raster,
                                   ORIENTATION_TOPLEFT, 0)) {
        _TIFFfree(raster);
        return (0);
    }

    /*
     * XXX: raster array has 4-byte unsigned integer type, that is why
     * we should rearrange it here.
     */
#if HOST_BIGENDIAN
    TIFFSwabArrayOfLong(raster, width * height);
#endif

    /*
     * Do we want to strip away alpha components?
     */
    if (no_alpha)
    {
        size_t count = pixel_count;
        unsigned char *src, *dst;

	src = dst = (unsigned char *) raster;
        while (count > 0)
        {
	    *(dst++) = *(src++);
	    *(dst++) = *(src++);
	    *(dst++) = *(src++);
	    src++;
	    count--;
        }
    }

    /*
     * Write out the result in strips
     */
    for (row = 0; row < height; row += rowsperstrip)
    {
        unsigned char * raster_strip;
        int	rows_to_write;
        int	bytes_per_pixel;

        if (no_alpha)
        {
            raster_strip = ((unsigned char *) raster) + 3 * row * width;
            bytes_per_pixel = 3;
        }
        else
        {
            raster_strip = (unsigned char *) (raster + row * width);
            bytes_per_pixel = 4;
        }

        if( row + rowsperstrip > height )
            rows_to_write = height - row;
        else
            rows_to_write = rowsperstrip;

        if( TIFFWriteEncodedStrip( out, row / rowsperstrip, raster_strip,
                             bytes_per_pixel * rows_to_write * width ) == -1 )
        {
            _TIFFfree( raster );
            return 0;
        }
    }

    _TIFFfree( raster );

    return 1;
}