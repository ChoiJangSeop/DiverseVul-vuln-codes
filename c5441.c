tiff_image_parse (TIFF *tiff, TiffContext *context, GError **error)
{
	guchar *pixels = NULL;
	gint width, height, rowstride, bytes;
	GdkPixbuf *pixbuf;
	guint16 bits_per_sample = 0;
	uint16 orientation = 0;
	uint16 transform = 0;
        uint16 codec;
        gchar *icc_profile_base64;
        const gchar *icc_profile;
        guint icc_profile_size;
        uint16 resolution_unit;
        gchar *density_str;
        gint retval;

        /* We're called with the lock held. */

	if (!TIFFGetField (tiff, TIFFTAG_IMAGEWIDTH, &width)) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_FAILED,
                                     _("Could not get image width (bad TIFF file)"));
                return NULL;
        }
        
        if (!TIFFGetField (tiff, TIFFTAG_IMAGELENGTH, &height)) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_FAILED,
                                     _("Could not get image height (bad TIFF file)"));
                return NULL;
        }

        if (width <= 0 || height <= 0) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("Width or height of TIFF image is zero"));
                return NULL;                
        }
        
        rowstride = width * 4;
        if (rowstride / 4 != width) { /* overflow */
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("Dimensions of TIFF image too large"));
                return NULL;                
        }
        
        bytes = height * rowstride;
        if (bytes / rowstride != height) { /* overflow */
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("Dimensions of TIFF image too large"));
                return NULL;                
        }

	if (context && context->size_func) {
                gint w = width;
                gint h = height;
		(* context->size_func) (&w, &h, context->user_data);
                
		/* This is a signal that this function is being called
		   to support gdk_pixbuf_get_file_info, so we can stop
		   parsing the tiff file at this point. It is not an
		   error condition. */

                if (w == 0 || h == 0)
                    return NULL;
        }

        pixels = g_try_malloc (bytes);

        if (!pixels) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                     _("Insufficient memory to open TIFF file"));
                return NULL;
        }

	pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, TRUE, 8, 
                                           width, height, rowstride,
                                           free_buffer, NULL);
        if (!pixbuf) {
                g_free (pixels);
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                     _("Insufficient memory to open TIFF file"));
                return NULL;
        }

        /* Save the bits per sample as an option since pixbufs are
           expected to be always 8 bits per sample. */
        TIFFGetField (tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
        if (bits_per_sample > 0) {
                gchar str[5];
                g_snprintf (str, sizeof (str), "%d", bits_per_sample);
                gdk_pixbuf_set_option (pixbuf, "bits-per-sample", str);
        }

	/* Set the "orientation" key associated with this image. libtiff 
	   orientation handling is odd, so further processing is required
	   by higher-level functions based on this tag. If the embedded
	   orientation tag is 1-4, libtiff flips/mirrors the image as
	   required, and no client processing is required - so we report 
	   no orientation. Orientations 5-8 require rotations which would 
	   swap the width and height of the image. libtiff does not do this. 
	   Instead it interprets orientations 5-8 the same as 1-4. 
	   See http://bugzilla.remotesensing.org/show_bug.cgi?id=1548.
	   To correct for this, the client must apply the transform normally
	   used for orientation 5 to both orientations 5 and 7, and apply
	   the transform normally used for orientation 7 for both
	   orientations 6 and 8. Then everythings works out OK! */
	
	TIFFGetField (tiff, TIFFTAG_ORIENTATION, &orientation);

	switch (orientation) {
		case 5:
		case 7:
			transform = 5;
			break;
		case 6:
		case 8:
			transform = 7;
			break;
		default:
			transform = 0;
			break;
	}

	if (transform > 0 ) {
		gchar str[5];
		g_snprintf (str, sizeof (str), "%d", transform);
		gdk_pixbuf_set_option (pixbuf, "orientation", str);
	}

        TIFFGetField (tiff, TIFFTAG_COMPRESSION, &codec);
        if (codec > 0) {
          gchar str[5];
          g_snprintf (str, sizeof (str), "%d", codec);
          gdk_pixbuf_set_option (pixbuf, "compression", str);
        }

        /* Extract embedded ICC profile */
        retval = TIFFGetField (tiff, TIFFTAG_ICCPROFILE, &icc_profile_size, &icc_profile);
        if (retval == 1) {
                icc_profile_base64 = g_base64_encode ((const guchar *) icc_profile, icc_profile_size);
                gdk_pixbuf_set_option (pixbuf, "icc-profile", icc_profile_base64);
                g_free (icc_profile_base64);
        }

        retval = TIFFGetField (tiff, TIFFTAG_RESOLUTIONUNIT, &resolution_unit);
        if (retval == 1) {
                float x_resolution = 0, y_resolution = 0;

                TIFFGetField (tiff, TIFFTAG_XRESOLUTION, &x_resolution);
                TIFFGetField (tiff, TIFFTAG_YRESOLUTION, &y_resolution);

                switch (resolution_unit) {
                case RESUNIT_INCH:
                        density_str = g_strdup_printf ("%d", (int) round (x_resolution));
                        gdk_pixbuf_set_option (pixbuf, "x-dpi", density_str);
                        g_free (density_str);
                        density_str = g_strdup_printf ("%d", (int) round (y_resolution));
                        gdk_pixbuf_set_option (pixbuf, "y-dpi", density_str);
                        g_free (density_str);
                        break;
                case RESUNIT_CENTIMETER:
                        density_str = g_strdup_printf ("%d", DPCM_TO_DPI (x_resolution));
                        gdk_pixbuf_set_option (pixbuf, "x-dpi", density_str);
                        g_free (density_str);
                        density_str = g_strdup_printf ("%d", DPCM_TO_DPI (y_resolution));
                        gdk_pixbuf_set_option (pixbuf, "y-dpi", density_str);
                        g_free (density_str);
                        break;
                }
        }

	if (context && context->prepare_func)
		(* context->prepare_func) (pixbuf, NULL, context->user_data);

	if (!TIFFReadRGBAImageOriented (tiff, width, height, (uint32 *)pixels, ORIENTATION_TOPLEFT, 1)) {
		g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_FAILED,
                                     _("Failed to load RGB data from TIFF file"));
		g_object_unref (pixbuf);
		return NULL;
	}

	/* Flag multi-page documents, because this loader only handles the
	   first page. The client app may wish to warn the user. */
        if (TIFFReadDirectory (tiff))
                gdk_pixbuf_set_option (pixbuf, "multipage", "yes");

#if G_BYTE_ORDER == G_BIG_ENDIAN
	/* Turns out that the packing used by TIFFRGBAImage depends on 
         * the host byte order... 
         */ 
	while (pixels < pixbuf->pixels + bytes) {
		uint32 pixel = *(uint32 *)pixels;
		int r = TIFFGetR(pixel);
		int g = TIFFGetG(pixel);
		int b = TIFFGetB(pixel);
		int a = TIFFGetA(pixel);
		*pixels++ = r;
		*pixels++ = g;
		*pixels++ = b;
		*pixels++ = a;
	}
#endif

	if (context && context->update_func)
		(* context->update_func) (pixbuf, 0, 0, width, height, context->user_data);

        return pixbuf;
}