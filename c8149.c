gtStripSeparate(TIFFRGBAImage* img, uint32* raster, uint32 w, uint32 h)
{
	TIFF* tif = img->tif;
	tileSeparateRoutine put = img->put.separate;
	unsigned char *buf = NULL;
	unsigned char *p0 = NULL, *p1 = NULL, *p2 = NULL, *pa = NULL;
	uint32 row, y, nrow, rowstoread;
	tmsize_t pos;
	tmsize_t scanline;
	uint32 rowsperstrip, offset_row;
	uint32 imagewidth = img->width;
	tmsize_t stripsize;
	tmsize_t bufsize;
	int32 fromskew, toskew;
	int alpha = img->alpha;
	int ret = 1, flip;
        uint16 colorchannels;

	stripsize = TIFFStripSize(tif);  
	bufsize = _TIFFMultiplySSize(tif,alpha?4:3,stripsize, "gtStripSeparate");
	if (bufsize == 0) {
		return (0);
	}

	flip = setorientation(img);
	if (flip & FLIP_VERTICALLY) {
		y = h - 1;
		toskew = -(int32)(w + w);
	}
	else {
		y = 0;
		toskew = -(int32)(w - w);
	}

        switch( img->photometric )
        {
          case PHOTOMETRIC_MINISWHITE:
          case PHOTOMETRIC_MINISBLACK:
          case PHOTOMETRIC_PALETTE:
            colorchannels = 1;
            break;

          default:
            colorchannels = 3;
            break;
        }

	TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
	scanline = TIFFScanlineSize(tif);  
	fromskew = (w < imagewidth ? imagewidth - w : 0);
	for (row = 0; row < h; row += nrow)
	{
		rowstoread = rowsperstrip - (row + img->row_offset) % rowsperstrip;
		nrow = (row + rowstoread > h ? h - row : rowstoread);
		offset_row = row + img->row_offset;
                if( buf == NULL )
                {
                    if (_TIFFReadEncodedStripAndAllocBuffer(
                            tif, TIFFComputeStrip(tif, offset_row, 0),
                            (void**) &buf, bufsize,
                            ((row + img->row_offset)%rowsperstrip + nrow) * scanline)==(tmsize_t)(-1)
                        && (buf == NULL || img->stoponerr))
                    {
                            ret = 0;
                            break;
                    }
                    p0 = buf;
                    if( colorchannels == 1 )
                    {
                        p2 = p1 = p0;
                        pa = (alpha?(p0+3*stripsize):NULL);
                    }
                    else
                    {
                        p1 = p0 + stripsize;
                        p2 = p1 + stripsize;
                        pa = (alpha?(p2+stripsize):NULL);
                    }
                }
		else if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 0),
		    p0, ((row + img->row_offset)%rowsperstrip + nrow) * scanline)==(tmsize_t)(-1)
		    && img->stoponerr)
		{
			ret = 0;
			break;
		}
		if (colorchannels > 1 
                    && TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 1),
                                            p1, ((row + img->row_offset)%rowsperstrip + nrow) * scanline) == (tmsize_t)(-1)
		    && img->stoponerr)
		{
			ret = 0;
			break;
		}
		if (colorchannels > 1 
                    && TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 2),
                                            p2, ((row + img->row_offset)%rowsperstrip + nrow) * scanline) == (tmsize_t)(-1)
		    && img->stoponerr)
		{
			ret = 0;
			break;
		}
		if (alpha)
		{
			if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, colorchannels),
			    pa, ((row + img->row_offset)%rowsperstrip + nrow) * scanline)==(tmsize_t)(-1)
			    && img->stoponerr)
			{
				ret = 0;
				break;
			}
		}

		pos = ((row + img->row_offset) % rowsperstrip) * scanline + \
			((tmsize_t) img->col_offset * img->samplesperpixel);
		(*put)(img, raster+y*w, 0, y, w, nrow, fromskew, toskew, p0 + pos, p1 + pos,
		    p2 + pos, (alpha?(pa+pos):NULL));
		y += ((flip & FLIP_VERTICALLY) ? -(int32) nrow : (int32) nrow);
	}

	if (flip & FLIP_HORIZONTALLY) {
		uint32 line;

		for (line = 0; line < h; line++) {
			uint32 *left = raster + (line * w);
			uint32 *right = left + w - 1;

			while ( left < right ) {
				uint32 temp = *left;
				*left = *right;
				*right = temp;
				left++;
				right--;
			}
		}
	}

	_TIFFfree(buf);
	return (ret);
}