BGD_DECLARE(void) gdImagePngCtxEx (gdImagePtr im, gdIOCtx * outfile, int level)
{
	int i, j, bit_depth = 0, interlace_type;
	int width = im->sx;
	int height = im->sy;
	int colors = im->colorsTotal;
	int *open = im->open;
	int mapping[gdMaxColors];	/* mapping[gd_index] == png_index */
	png_byte trans_values[256];
	png_color_16 trans_rgb_value;
	png_color palette[gdMaxColors];
	png_structp png_ptr;
	png_infop info_ptr;
	volatile int transparent = im->transparent;
	volatile int remap = FALSE;
#ifdef PNG_SETJMP_SUPPORTED
	jmpbuf_wrapper jbw;
#endif

	/* width or height of value 0 is invalid in IHDR;
	   see http://www.w3.org/TR/PNG-Chunks.html */
	if (width == 0 || height ==0) return;

#ifdef PNG_SETJMP_SUPPORTED
	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
	                                   &jbw, gdPngErrorHandler,
	                                   NULL);
#else
	png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
#endif
	if (png_ptr == NULL) {
		gd_error("gd-png error: cannot allocate libpng main struct\n");
		return;
	}

	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		gd_error("gd-png error: cannot allocate libpng info struct\n");
		png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
		return;
	}

#ifdef PNG_SETJMP_SUPPORTED
	if (setjmp(jbw.jmpbuf)) {
		gd_error("gd-png error: setjmp returns error condition\n");
		png_destroy_write_struct (&png_ptr, &info_ptr);
		return;
	}
#endif

	png_set_write_fn (png_ptr, (void *) outfile, gdPngWriteData,
	                  gdPngFlushData);

	/* This is best for palette images, and libpng defaults to it for
	   palette images anyway, so we don't need to do it explicitly.
	   What to ideally do for truecolor images depends, alas, on the image.
	   gd is intentionally imperfect and doesn't spend a lot of time
	   fussing with such things. */

	/* Faster if this is uncommented, but may produce larger truecolor files.
	   Wait for gdImagePngCtxEx. */
#if 0
	png_set_filter (png_ptr, 0, PNG_FILTER_NONE);
#endif

	/* 2.0.12: this is finally a parameter */
	png_set_compression_level (png_ptr, level);

#ifdef PNG_pHYs_SUPPORTED
	/* 2.1.0: specify the resolution */
	png_set_pHYs(png_ptr, info_ptr, DPI2DPM(im->res_x), DPI2DPM(im->res_y),
	             PNG_RESOLUTION_METER);
#endif

	/* can set this to a smaller value without compromising compression if all
	 * image data is 16K or less; will save some decoder memory [min == 8] */
	/*  png_set_compression_window_bits(png_ptr, 15);  */

	if (!im->trueColor) {
		if (transparent >= im->colorsTotal ||
		        (transparent >= 0 && open[transparent]))
			transparent = -1;
	}
	if (!im->trueColor) {
		for (i = 0; i < gdMaxColors; ++i)
			mapping[i] = -1;
	}
	if (!im->trueColor) {
		/* count actual number of colors used (colorsTotal == high-water mark) */
		colors = 0;
		for (i = 0; i < im->colorsTotal; ++i) {
			if (!open[i]) {
				mapping[i] = colors;
				++colors;
			}
		}
		if (colors == 0) {
			gd_error("gd-png error: no colors in palette\n");
			goto bail;
		}
		if (colors < im->colorsTotal) {
			remap = TRUE;
		}
		if (colors <= 2)
			bit_depth = 1;
		else if (colors <= 4)
			bit_depth = 2;
		else if (colors <= 16)
			bit_depth = 4;
		else
			bit_depth = 8;
	}
	interlace_type = im->interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE;

	if (im->trueColor) {
		if (im->saveAlphaFlag) {
			png_set_IHDR (png_ptr, info_ptr, width, height, 8,
			              PNG_COLOR_TYPE_RGB_ALPHA, interlace_type,
			              PNG_COMPRESSION_TYPE_DEFAULT,
			              PNG_FILTER_TYPE_DEFAULT);
		} else {
			png_set_IHDR (png_ptr, info_ptr, width, height, 8,
			              PNG_COLOR_TYPE_RGB, interlace_type,
			              PNG_COMPRESSION_TYPE_DEFAULT,
			              PNG_FILTER_TYPE_DEFAULT);
		}
	} else {
		png_set_IHDR (png_ptr, info_ptr, width, height, bit_depth,
		              PNG_COLOR_TYPE_PALETTE, interlace_type,
		              PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}
	if (im->trueColor && (!im->saveAlphaFlag) && (transparent >= 0)) {
		/* 2.0.9: fixed by Thomas Winzig */
		trans_rgb_value.red = gdTrueColorGetRed (im->transparent);
		trans_rgb_value.green = gdTrueColorGetGreen (im->transparent);
		trans_rgb_value.blue = gdTrueColorGetBlue (im->transparent);
		png_set_tRNS (png_ptr, info_ptr, 0, 0, &trans_rgb_value);
	}
	if (!im->trueColor) {
		/* Oy veh. Remap the PNG palette to put the
		   entries with interesting alpha channel
		   values first. This minimizes the size
		   of the tRNS chunk and thus the size
		   of the PNG file as a whole. */
		int tc = 0;
		int i;
		int j;
		int k;
		for (i = 0; (i < im->colorsTotal); i++) {
			if ((!im->open[i]) && (im->alpha[i] != gdAlphaOpaque)) {
				tc++;
			}
		}
		if (tc) {
#if 0
			for (i = 0; (i < im->colorsTotal); i++) {
				trans_values[i] = 255 -
				                  ((im->alpha[i] << 1) + (im->alpha[i] >> 6));
			}
			png_set_tRNS (png_ptr, info_ptr, trans_values, 256, NULL);
#endif
			if (!remap) {
				remap = TRUE;
			}
			/* (Semi-)transparent indexes come up from the bottom
			   of the list of real colors; opaque
			   indexes come down from the top */
			j = 0;
			k = colors - 1;
			for (i = 0; (i < im->colorsTotal); i++) {
				if (!im->open[i]) {
					if (im->alpha[i] != gdAlphaOpaque) {
						/* Andrew Hull: >> 6, not >> 7! (gd 2.0.5) */
						trans_values[j] = 255 -
						                  ((im->alpha[i] << 1) + (im->alpha[i] >> 6));
						mapping[i] = j++;
					} else {
						mapping[i] = k--;
					}
				}
			}
			png_set_tRNS (png_ptr, info_ptr, trans_values, tc, NULL);
		}
	}

	/* convert palette to libpng layout */
	if (!im->trueColor) {
		if (remap)
			for (i = 0; i < im->colorsTotal; ++i) {
				if (mapping[i] < 0)
					continue;
				palette[mapping[i]].red = im->red[i];
				palette[mapping[i]].green = im->green[i];
				palette[mapping[i]].blue = im->blue[i];
			}
		else
			for (i = 0; i < colors; ++i) {
				palette[i].red = im->red[i];
				palette[i].green = im->green[i];
				palette[i].blue = im->blue[i];
			}
		png_set_PLTE (png_ptr, info_ptr, palette, colors);
	}

	/* write out the PNG header info (everything up to first IDAT) */
	png_write_info (png_ptr, info_ptr);

	/* make sure < 8-bit images are packed into pixels as tightly as possible */
	png_set_packing (png_ptr);

	/* This code allocates a set of row buffers and copies the gd image data
	 * into them only in the case that remapping is necessary; in gd 1.3 and
	 * later, the im->pixels array is laid out identically to libpng's row
	 * pointers and can be passed to png_write_image() function directly.
	 * The remapping case could be accomplished with less memory for non-
	 * interlaced images, but interlacing causes some serious complications. */
	if (im->trueColor) {
		/* performance optimizations by Phong Tran */
		int channels = im->saveAlphaFlag ? 4 : 3;
		/* Our little 7-bit alpha channel trick costs us a bit here. */
		png_bytep *row_pointers;
		unsigned char *pOutputRow;
		int **ptpixels = im->tpixels;
		int *pThisRow;
		unsigned char a;
		int thisPixel;
		png_bytep *prow_pointers;
		int saveAlphaFlag = im->saveAlphaFlag;
		if (overflow2(sizeof (png_bytep), height)) {
			goto bail;
		}
		row_pointers = gdMalloc (sizeof (png_bytep) * height);
		if (row_pointers == NULL) {
			gd_error("gd-png error: unable to allocate row_pointers\n");
			goto bail;
		}
		prow_pointers = row_pointers;
		for (j = 0; j < height; ++j) {
			if (overflow2(width, channels) || ((*prow_pointers =
			                                        (png_bytep) gdMalloc (width * channels)) == NULL)) {
				gd_error("gd-png error: unable to allocate rows\n");
				for (i = 0; i < j; ++i)
					gdFree (row_pointers[i]);
				/* 2.0.29: memory leak TBB */
				gdFree(row_pointers);
				goto bail;
			}
			pOutputRow = *prow_pointers++;
			pThisRow = *ptpixels++;
			for (i = 0; i < width; ++i) {
				thisPixel = *pThisRow++;
				*pOutputRow++ = gdTrueColorGetRed (thisPixel);
				*pOutputRow++ = gdTrueColorGetGreen (thisPixel);
				*pOutputRow++ = gdTrueColorGetBlue (thisPixel);

				if (saveAlphaFlag) {
					/* convert the 7-bit alpha channel to an 8-bit alpha channel.
					   We do a little bit-flipping magic, repeating the MSB
					   as the LSB, to ensure that 0 maps to 0 and
					   127 maps to 255. We also have to invert to match
					   PNG's convention in which 255 is opaque. */
					a = gdTrueColorGetAlpha (thisPixel);
					/* Andrew Hull: >> 6, not >> 7! (gd 2.0.5) */
					*pOutputRow++ = 255 - ((a << 1) + (a >> 6));
				}
			}
		}

		png_write_image (png_ptr, row_pointers);
		png_write_end (png_ptr, info_ptr);

		for (j = 0; j < height; ++j)
			gdFree (row_pointers[j]);
		gdFree (row_pointers);
	} else {
		if (remap) {
			png_bytep *row_pointers;
			if (overflow2(sizeof (png_bytep), height)) {
				goto bail;
			}
			row_pointers = gdMalloc (sizeof (png_bytep) * height);
			if (row_pointers == NULL) {
				gd_error("gd-png error: unable to allocate row_pointers\n");
				goto bail;
			}
			for (j = 0; j < height; ++j) {
				if ((row_pointers[j] = (png_bytep) gdMalloc (width)) == NULL) {
					gd_error("gd-png error: unable to allocate rows\n");
					for (i = 0; i < j; ++i)
						gdFree (row_pointers[i]);
					/* TBB: memory leak */
					gdFree (row_pointers);
					goto bail;
				}
				for (i = 0; i < width; ++i)
					row_pointers[j][i] = mapping[im->pixels[j][i]];
			}

			png_write_image (png_ptr, row_pointers);
			png_write_end (png_ptr, info_ptr);

			for (j = 0; j < height; ++j)
				gdFree (row_pointers[j]);
			gdFree (row_pointers);
		} else {
			png_write_image (png_ptr, im->pixels);
			png_write_end (png_ptr, info_ptr);
		}
	}
	/* 1.6.3: maybe we should give that memory BACK! TBB */
bail:
	png_destroy_write_struct (&png_ptr, &info_ptr);
}