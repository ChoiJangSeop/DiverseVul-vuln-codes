main(int argc, char* argv[])
{
	uint32 rowsperstrip = (uint32) -1;
	TIFF *in, *out;
	uint32 w, h;
	uint16 samplesperpixel;
	uint16 bitspersample;
	uint16 config;
	uint16 photometric;
	uint16* red;
	uint16* green;
	uint16* blue;
	tsize_t rowsize;
	register uint32 row;
	register tsample_t s;
	unsigned char *inbuf, *outbuf;
	char thing[1024];
	int c;
#if !HAVE_DECL_OPTARG
	extern int optind;
	extern char *optarg;
#endif
        
        in = (TIFF *) NULL;
        out = (TIFF *) NULL;
        inbuf = (unsigned char *) NULL;
        outbuf = (unsigned char *) NULL;

	while ((c = getopt(argc, argv, "c:r:R:G:B:")) != -1)
		switch (c) {
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage();
			break;
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
			break;
		case 'R':
			RED = PCT(atoi(optarg));
			break;
		case 'G':
			GREEN = PCT(atoi(optarg));
			break;
		case 'B':
			BLUE = PCT(atoi(optarg));
			break;
		case '?':
			usage();
			/*NOTREACHED*/
		}
	if (argc - optind < 2)
		usage();
	in = TIFFOpen(argv[optind], "r");
	if (in == NULL)
		return (-1);
	photometric = 0;
	TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photometric);
	if (photometric != PHOTOMETRIC_RGB && photometric != PHOTOMETRIC_PALETTE ) {
		fprintf(stderr,
	    "%s: Bad photometric; can only handle RGB and Palette images.\n",
		    argv[optind]);
                goto tiff2bw_error;
	}
	TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
	if (samplesperpixel != 1 && samplesperpixel != 3) {
		fprintf(stderr, "%s: Bad samples/pixel %u.\n",
		    argv[optind], samplesperpixel);
                goto tiff2bw_error;
	}
	if( photometric == PHOTOMETRIC_RGB && samplesperpixel != 3) {
		fprintf(stderr, "%s: Bad samples/pixel %u for PHOTOMETRIC_RGB.\n",
		    argv[optind], samplesperpixel);
                goto tiff2bw_error;
	}
	TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	if (bitspersample != 8) {
		fprintf(stderr,
		    " %s: Sorry, only handle 8-bit samples.\n", argv[optind]);
                goto tiff2bw_error;
	}
	TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(in, TIFFTAG_IMAGELENGTH, &h);
	TIFFGetField(in, TIFFTAG_PLANARCONFIG, &config);

	out = TIFFOpen(argv[optind+1], "w");
	if (out == NULL)
	{
                goto tiff2bw_error;
	}
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, w);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	cpTags(in, out);
	if (compression != (uint16) -1) {
		TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
		switch (compression) {
		case COMPRESSION_JPEG:
			TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
			TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, jpegcolormode);
			break;
		case COMPRESSION_LZW:
		case COMPRESSION_DEFLATE:
			if (predictor != 0)
				TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
			break;
		}
	}
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	snprintf(thing, sizeof(thing), "B&W version of %s", argv[optind]);
	TIFFSetField(out, TIFFTAG_IMAGEDESCRIPTION, thing);
	TIFFSetField(out, TIFFTAG_SOFTWARE, "tiff2bw");
	outbuf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
	    TIFFDefaultStripSize(out, rowsperstrip));

#define	pack(a,b)	((a)<<8 | (b))
	switch (pack(photometric, config)) {
	case pack(PHOTOMETRIC_PALETTE, PLANARCONFIG_CONTIG):
	case pack(PHOTOMETRIC_PALETTE, PLANARCONFIG_SEPARATE):
		TIFFGetField(in, TIFFTAG_COLORMAP, &red, &green, &blue);
		/*
		 * Convert 16-bit colormap to 8-bit (unless it looks
		 * like an old-style 8-bit colormap).
		 */
		if (checkcmap(in, 1<<bitspersample, red, green, blue) == 16) {
			int i;
#define	CVT(x)		(((x) * 255L) / ((1L<<16)-1))
			for (i = (1<<bitspersample)-1; i >= 0; i--) {
				red[i] = CVT(red[i]);
				green[i] = CVT(green[i]);
				blue[i] = CVT(blue[i]);
			}
#undef CVT
		}
		inbuf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(in));
		for (row = 0; row < h; row++) {
			if (TIFFReadScanline(in, inbuf, row, 0) < 0)
				break;
			compresspalette(outbuf, inbuf, w, red, green, blue);
			if (TIFFWriteScanline(out, outbuf, row, 0) < 0)
				break;
		}
		break;
	case pack(PHOTOMETRIC_RGB, PLANARCONFIG_CONTIG):
		inbuf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(in));
		for (row = 0; row < h; row++) {
			if (TIFFReadScanline(in, inbuf, row, 0) < 0)
				break;
			compresscontig(outbuf, inbuf, w);
			if (TIFFWriteScanline(out, outbuf, row, 0) < 0)
				break;
		}
		break;
	case pack(PHOTOMETRIC_RGB, PLANARCONFIG_SEPARATE):
		rowsize = TIFFScanlineSize(in);
		inbuf = (unsigned char *)_TIFFmalloc(3*rowsize);
		for (row = 0; row < h; row++) {
			for (s = 0; s < 3; s++)
				if (TIFFReadScanline(in,
				    inbuf+s*rowsize, row, s) < 0)
                                        goto tiff2bw_error;
			compresssep(outbuf,
			    inbuf, inbuf+rowsize, inbuf+2*rowsize, w);
			if (TIFFWriteScanline(out, outbuf, row, 0) < 0)
				break;
		}
		break;
	}
#undef pack
        if (inbuf)
                _TIFFfree(inbuf);
        if (outbuf)
                _TIFFfree(outbuf);
        TIFFClose(in);
	TIFFClose(out);
	return (0);

 tiff2bw_error:
        if (inbuf)
                _TIFFfree(inbuf);
        if (outbuf)
                _TIFFfree(outbuf);
        if (out)
                TIFFClose(out);
        if (in)
                TIFFClose(in);
        return (-1);
}