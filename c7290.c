static gboolean DecodeHeader(unsigned char *BFH, unsigned char *BIH,
                             struct bmp_progressive_state *State,
                             GError **error)
{
	gint clrUsed;

	/* First check for the two first bytes content. A sane
	   BMP file must start with bytes 0x42 0x4D.  */
	if (*BFH != 0x42 || *(BFH + 1) != 0x4D) {
		g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("BMP image has bogus header data"));
		State->read_state = READ_STATE_ERROR;
		return FALSE;
	}

        /* FIXME this is totally unrobust against bogus image data. */
	if (State->BufferSize < lsb_32 (&BIH[0]) + 14) {
		State->BufferSize = lsb_32 (&BIH[0]) + 14;
		if (!grow_buffer (State, error))
			return FALSE;
		return TRUE;
	}

#if DUMPBIH
	DumpBIH(BIH);
#endif    

	State->Header.size = lsb_32 (&BIH[0]);
	if (State->Header.size == 124) {
                /* BMP v5 */
		State->Header.width = lsb_32 (&BIH[4]);
		State->Header.height = lsb_32 (&BIH[8]);
		State->Header.depth = lsb_16 (&BIH[14]);
		State->Compressed = lsb_32 (&BIH[16]);
	} else if (State->Header.size == 108) {
                /* BMP v4 */
		State->Header.width = lsb_32 (&BIH[4]);
		State->Header.height = lsb_32 (&BIH[8]);
		State->Header.depth = lsb_16 (&BIH[14]);
		State->Compressed = lsb_32 (&BIH[16]);
	} else if (State->Header.size == 64) {
                /* BMP OS/2 v2 */
		State->Header.width = lsb_32 (&BIH[4]);
		State->Header.height = lsb_32 (&BIH[8]);
		State->Header.depth = lsb_16 (&BIH[14]);
		State->Compressed = lsb_32 (&BIH[16]);
	} else if (State->Header.size == 40) {
                /* BMP v3 */ 
		State->Header.width = lsb_32 (&BIH[4]);
		State->Header.height = lsb_32 (&BIH[8]);
		State->Header.depth = lsb_16 (&BIH[14]);
		State->Compressed = lsb_32 (&BIH[16]);
	} else if (State->Header.size == 12) {
                /* BMP OS/2 */
		State->Header.width = lsb_16 (&BIH[4]);
		State->Header.height = lsb_16 (&BIH[6]);
		State->Header.depth = lsb_16 (&BIH[10]);
		State->Compressed = BI_RGB;
	} else {
		g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("BMP image has unsupported header size"));
		State->read_state = READ_STATE_ERROR;
		return FALSE;
	}

        if (State->Header.depth > 32)
          {
		g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("BMP image has unsupported depth"));
		State->read_state = READ_STATE_ERROR;
          }

	if (State->Header.size == 12)
		clrUsed = 1 << State->Header.depth;
	else
		clrUsed = (int) (BIH[35] << 24) + (BIH[34] << 16) + (BIH[33] << 8) + (BIH[32]);

	if (clrUsed != 0)
		State->Header.n_colors = clrUsed;
	else
            State->Header.n_colors = (1 << State->Header.depth);

	State->Type = State->Header.depth;	/* This may be less trivial someday */

	/* Negative heights indicates bottom-down pixelorder */
	if (State->Header.height < 0) {
		State->Header.height = -State->Header.height;
		State->Header.Negative = 1;
	}

	if (State->Header.Negative && 
	    (State->Compressed != BI_RGB && State->Compressed != BI_BITFIELDS))
	{
		g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("Topdown BMP images cannot be compressed"));
		State->read_state = READ_STATE_ERROR;
		return FALSE;
	}

	if (State->Header.width <= 0 || State->Header.height == 0 ||
	    (State->Compressed == BI_RLE4 && State->Type != 4)    ||
	    (State->Compressed == BI_RLE8 && State->Type != 8)	  ||
	    (State->Compressed == BI_BITFIELDS && !(State->Type == 16 || State->Type == 32)) ||
	    (State->Compressed > BI_BITFIELDS)) {
		g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("BMP image has bogus header data"));
		State->read_state = READ_STATE_ERROR;
		return FALSE;
	}

	if (State->Type == 32)
		State->LineWidth = State->Header.width * 4;
	else if (State->Type == 24)
		State->LineWidth = State->Header.width * 3;
	else if (State->Type == 16)
		State->LineWidth = State->Header.width * 2;
	else if (State->Type == 8)
		State->LineWidth = State->Header.width * 1;
	else if (State->Type == 4)
		State->LineWidth = (State->Header.width + 1) / 2;
	else if (State->Type == 1) {
		State->LineWidth = State->Header.width / 8;
		if ((State->Header.width & 7) != 0)
			State->LineWidth++;
	} else {
		g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("BMP image has bogus header data"));
		State->read_state = READ_STATE_ERROR;
		return FALSE;
	}

	/* Pad to a 32 bit boundary */
	if (((State->LineWidth % 4) > 0)
	    && (State->Compressed == BI_RGB || State->Compressed == BI_BITFIELDS))
		State->LineWidth = (State->LineWidth / 4) * 4 + 4;

	if (State->pixbuf == NULL) {
		if (State->size_func) {
			gint width = State->Header.width;
			gint height = State->Header.height;

			(*State->size_func) (&width, &height, State->user_data);
			if (width == 0 || height == 0) {
				State->read_state = READ_STATE_DONE;
				State->BufferSize = 0;
				return TRUE;
			}
		}

		if (State->Type == 32 || 
		    State->Compressed == BI_RLE4 || 
		    State->Compressed == BI_RLE8)
			State->pixbuf =
				gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
					       (gint) State->Header.width,
					       (gint) State->Header.height);
		else
			State->pixbuf =
				gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8,
					       (gint) State->Header.width,
					       (gint) State->Header.height);
		
		if (State->pixbuf == NULL) {
			g_set_error_literal (error,
                                             GDK_PIXBUF_ERROR,
                                             GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                             _("Not enough memory to load bitmap image"));
			State->read_state = READ_STATE_ERROR;
			return FALSE;
			}
		
		if (State->prepared_func != NULL)
			/* Notify the client that we are ready to go */
			(*State->prepared_func) (State->pixbuf, NULL, State->user_data);
		
		/* make all pixels initially transparent */
		if (State->Compressed == BI_RLE4 || State->Compressed == BI_RLE8) {
			memset (State->pixbuf->pixels, 0, State->pixbuf->rowstride * State->Header.height);
			State->compr.p = State->pixbuf->pixels 
				+ State->pixbuf->rowstride * (State->Header.height- 1);
		}
	}
	
	State->BufferDone = 0;
	if (State->Type <= 8) {
                gint samples;

		State->read_state = READ_STATE_PALETTE;

		/* Allocate enough to hold the palette */
	        samples = (State->Header.size == 12 ? 3 : 4);
		State->BufferSize = State->Header.n_colors * samples;

		/* Skip over everything between the palette and the data.
		   This protects us against a malicious BFH[10] value.
                */
		State->BufferPadding = (lsb_32 (&BFH[10]) - 14 - State->Header.size) - State->BufferSize;

	} else if (State->Compressed == BI_RGB) {
		if (State->BufferSize < lsb_32 (&BFH[10]))
		{
			/* skip over padding between headers and image data */
			State->read_state = READ_STATE_HEADERS;
			State->BufferDone = State->BufferSize;
			State->BufferSize = lsb_32 (&BFH[10]);
		}
		else
		{
			State->read_state = READ_STATE_DATA;
			State->BufferSize = State->LineWidth;
		}
	} else if (State->Compressed == BI_BITFIELDS) {
               if (State->Header.size == 108 || State->Header.size == 124) 
               {
			/* v4 and v5 have the bitmasks in the header */
			if (!decode_bitmasks (&BIH[40], State, error)) {
			       State->read_state = READ_STATE_ERROR;
			       return FALSE;
                        }
               }
               else 
               {
		       State->read_state = READ_STATE_BITMASKS;
		       State->BufferSize = 12;
               }
	} else {
		g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("BMP image has bogus header data"));
		State->read_state = READ_STATE_ERROR;
		return FALSE;
	}

	if (!grow_buffer (State, error)) 
		return FALSE;

        return TRUE;
}