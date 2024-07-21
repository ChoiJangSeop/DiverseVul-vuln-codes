gdk_pixbuf__tiff_image_load_real (FILE *f, TiffData *context, GError **error)
{
	TIFF *tiff;
	guchar *pixels = NULL;
	guchar *tmppix;
	gint w, h, x, y, num_pixs, fd;
	uint32 *rast, *tmp_rast;
	GdkPixbuf *pixbuf;
	
	fd = fileno (f);
	tiff = TIFFFdOpen (fd, "libpixbuf-tiff", "r");

	if (!tiff) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                             _("Failed to open TIFF image"));
		return NULL;
        }
                
	TIFFGetField (tiff, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField (tiff, TIFFTAG_IMAGELENGTH, &h);
	num_pixs = w * h;
	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, w, h);

        if (!pixbuf) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                             _("Insufficient memory to open TIFF file"));
		TIFFClose (tiff);
		return NULL;
        }
        
	if (context)
		(* context->prepare_func) (pixbuf, NULL, context->user_data);

	/* Yes, it needs to be _TIFFMalloc... */
	rast = (uint32 *) _TIFFmalloc (num_pixs * sizeof (uint32));

	if (!rast) {
                g_set_error (error,
                             GDK_PIXBUF_ERROR,
                             GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                             _("Insufficient memory to open TIFF file"));
		TIFFClose (tiff);
		return NULL;
	}

	if (TIFFReadRGBAImage (tiff, w, h, rast, 0)) {
		pixels = gdk_pixbuf_get_pixels (pixbuf);

                g_assert (pixels);
                
		tmppix = pixels;

		for (y = 0; y < h; y++) {
			/* Unexplainable...are tiffs backwards? */
			/* Also looking at the GIMP plugin, this
			 * whole reading thing can be a bit more
			 * robust.
			 */
			tmp_rast = rast + ((h - y - 1) * w);
			for (x = 0; x < w; x++) {
				tmppix[0] = TIFFGetR (*tmp_rast);
				tmppix[1] = TIFFGetG (*tmp_rast);
				tmppix[2] = TIFFGetB (*tmp_rast);
				tmppix[3] = TIFFGetA (*tmp_rast);
				tmp_rast++;
				tmppix += 4;
			}
		}
	}
	_TIFFfree (rast);
	TIFFClose (tiff);

	if (context) {
		(* context->update_func) (pixbuf, 0, 0, w, h, context->user_data);
		gdk_pixbuf_unref (pixbuf);
	}

	return pixbuf;
}