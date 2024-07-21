tiffcvt(TIFF* in, TIFF* out)
{
	uint32 width, height;		/* image width & height */
	uint16 shortv;
	float floatv;
	char *stringv;
	uint32 longv;
        uint16 v[1];

	TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(in, TIFFTAG_IMAGELENGTH, &height);

	CopyField(TIFFTAG_SUBFILETYPE, longv);
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	CopyField(TIFFTAG_FILLORDER, shortv);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

        if( no_alpha )
            TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 3);
        else
            TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 4);

        if( !no_alpha )
        {
            v[0] = EXTRASAMPLE_ASSOCALPHA;
            TIFFSetField(out, TIFFTAG_EXTRASAMPLES, 1, v);
        }

	CopyField(TIFFTAG_XRESOLUTION, floatv);
	CopyField(TIFFTAG_YRESOLUTION, floatv);
	CopyField(TIFFTAG_RESOLUTIONUNIT, shortv);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_SOFTWARE, TIFFGetVersion());
	CopyField(TIFFTAG_DOCUMENTNAME, stringv);

        if( process_by_block && TIFFIsTiled( in ) )
            return( cvt_by_tile( in, out ) );
        else if( process_by_block )
            return( cvt_by_strip( in, out ) );
        else
            return( cvt_whole_image( in, out ) );
}