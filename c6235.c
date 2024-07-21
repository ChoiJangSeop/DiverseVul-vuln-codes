gxps_images_create_from_tiff (GXPSArchive *zip,
			      const gchar *image_uri,
			      GError     **error)
{
#ifdef HAVE_LIBTIFF
	TIFF       *tiff;
	TiffBuffer  buffer;
	GXPSImage  *image;
	gint        width, height;
	guint16     res_unit;
	float       res_x, res_y;
	gint        stride;
	guchar     *data;
	guchar     *p;

	if (!gxps_archive_read_entry (zip, image_uri,
				      &buffer.buffer,
				      &buffer.buffer_len,
				      error)) {
		g_set_error (error,
			     GXPS_ERROR,
			     GXPS_ERROR_SOURCE_NOT_FOUND,
			     "Image source %s not found in archive",
			     image_uri);
		return NULL;
	}

	buffer.pos = 0;

	_tiff_push_handlers ();

	tiff = TIFFClientOpen ("libgxps-tiff", "r", &buffer,
			       _tiff_read,
			       _tiff_write,
			       _tiff_seek,
			       _tiff_close,
			       _tiff_size,
			       _tiff_map_file,
			       _tiff_unmap_file);

	if (!tiff || _tiff_error) {
		fill_tiff_error (error, image_uri);
		if (tiff)
			TIFFClose (tiff);
		_tiff_pop_handlers ();
		g_free (buffer.buffer);
		return NULL;
	}

	if (!TIFFGetField (tiff, TIFFTAG_IMAGEWIDTH, &width) || _tiff_error) {
		fill_tiff_error (error, image_uri);
		TIFFClose (tiff);
		_tiff_pop_handlers ();
		g_free (buffer.buffer);
		return NULL;
	}

	if (!TIFFGetField (tiff, TIFFTAG_IMAGELENGTH, &height) || _tiff_error) {
		fill_tiff_error (error, image_uri);
		TIFFClose (tiff);
		_tiff_pop_handlers ();
		g_free (buffer.buffer);
		return NULL;
	}

	if (width <= 0 || height <= 0) {
		fill_tiff_error (error, image_uri);
		TIFFClose (tiff);
		_tiff_pop_handlers ();
		g_free (buffer.buffer);
		return NULL;
	}

	image = g_slice_new (GXPSImage);
	image->surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
						     width, height);
	image->res_x = 96;
	image->res_y = 96;

	if (!TIFFGetField (tiff, TIFFTAG_RESOLUTIONUNIT, &res_unit))
		res_unit = 0;
	if (TIFFGetField (tiff, TIFFTAG_XRESOLUTION, &res_x)) {
		if (res_unit == 2) { /* inches */
			image->res_x = res_x;
		} else if (res_unit == 3) { /* centimeters */
			image->res_x = res_x * CENTIMETERS_PER_INCH;
		}
	}
	if (TIFFGetField (tiff, TIFFTAG_YRESOLUTION, &res_y)) {
		if (res_unit == 2) { /* inches */
			image->res_y = res_y;
		} else if (res_unit == 3) { /* centimeters */
			image->res_y = res_y * CENTIMETERS_PER_INCH;
		}
	}

	if (cairo_surface_status (image->surface)) {
		g_set_error (error,
			     GXPS_ERROR,
			     GXPS_ERROR_IMAGE,
			     "Error loading TIFF image %s: %s",
			     image_uri,
			     cairo_status_to_string (cairo_surface_status (image->surface)));
		gxps_image_free (image);
		TIFFClose (tiff);
		_tiff_pop_handlers ();
		g_free (buffer.buffer);
		return NULL;
	}

	data = cairo_image_surface_get_data (image->surface);
	if (!TIFFReadRGBAImageOriented (tiff, width, height,
					(uint32 *)data,
					ORIENTATION_TOPLEFT, 1) || _tiff_error) {
		fill_tiff_error (error, image_uri);
		gxps_image_free (image);
		TIFFClose (tiff);
		_tiff_pop_handlers ();
		g_free (buffer.buffer);
		return NULL;
	}

	TIFFClose (tiff);
	_tiff_pop_handlers ();
	g_free (buffer.buffer);

	stride = cairo_image_surface_get_stride (image->surface);
	p = data;
	while (p < data + (height * stride)) {
		guint32 *pixel = (guint32 *)p;
		guint8   r = TIFFGetR (*pixel);
		guint8   g = TIFFGetG (*pixel);
		guint8   b = TIFFGetB (*pixel);
		guint8   a = TIFFGetA (*pixel);

		*pixel = (a << 24) | (r << 16) | (g << 8) | b;

		p += 4;
	}

	cairo_surface_mark_dirty (image->surface);

	return image;
#else
	return NULL;
#endif /* #ifdef HAVE_LIBTIFF */
}