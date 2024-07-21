gxps_images_create_from_png (GXPSArchive *zip,
			     const gchar *image_uri,
			     GError     **error)
{
#ifdef HAVE_LIBPNG
	GInputStream  *stream;
	GXPSImage     *image = NULL;
	char          *png_err_msg = NULL;
	png_struct    *png;
	png_info      *info;
	png_byte      *data = NULL;
	png_byte     **row_pointers = NULL;
	png_uint_32    png_width, png_height;
	int            depth, color_type, interlace, stride;
	unsigned int   i;
	cairo_format_t format;
	cairo_status_t status;

	stream = gxps_archive_open (zip, image_uri);
	if (!stream) {
		g_set_error (error,
			     GXPS_ERROR,
			     GXPS_ERROR_SOURCE_NOT_FOUND,
			     "Image source %s not found in archive",
			     image_uri);
		return NULL;
	}

	png = png_create_read_struct (PNG_LIBPNG_VER_STRING,
				      &png_err_msg,
				      png_error_callback,
				      png_warning_callback);
	if (png == NULL) {
		fill_png_error (error, image_uri, NULL);
		g_object_unref (stream);
		return NULL;
	}

	info = png_create_info_struct (png);
	if (info == NULL) {
		fill_png_error (error, image_uri, NULL);
		g_object_unref (stream);
		png_destroy_read_struct (&png, NULL, NULL);
		return NULL;
	}

	png_set_read_fn (png, stream, _read_png);

	if (setjmp (png_jmpbuf (png))) {
		fill_png_error (error, image_uri, png_err_msg);
		g_free (png_err_msg);
		g_object_unref (stream);
		png_destroy_read_struct (&png, &info, NULL);
		gxps_image_free (image);
		g_free (row_pointers);
		g_free (data);
		return NULL;
	}

	png_read_info (png, info);

	png_get_IHDR (png, info,
		      &png_width, &png_height, &depth,
		      &color_type, &interlace, NULL, NULL);

	/* convert palette/gray image to rgb */
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb (png);

	/* expand gray bit depth if needed */
	if (color_type == PNG_COLOR_TYPE_GRAY)
		png_set_expand_gray_1_2_4_to_8 (png);

	/* transform transparency to alpha */
	if (png_get_valid (png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha (png);

	if (depth == 16)
		png_set_strip_16 (png);

	if (depth < 8)
		png_set_packing (png);

	/* convert grayscale to RGB */
	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb (png);

	if (interlace != PNG_INTERLACE_NONE)
		png_set_interlace_handling (png);

	png_set_filler (png, 0xff, PNG_FILLER_AFTER);

	/* recheck header after setting EXPAND options */
	png_read_update_info (png, info);
	png_get_IHDR (png, info,
		      &png_width, &png_height, &depth,
		      &color_type, &interlace, NULL, NULL);
	if (depth != 8 ||
	    !(color_type == PNG_COLOR_TYPE_RGB ||
              color_type == PNG_COLOR_TYPE_RGB_ALPHA)) {
		fill_png_error (error, image_uri, NULL);
		g_object_unref (stream);
		png_destroy_read_struct (&png, &info, NULL);
		return NULL;
	}

	switch (color_type) {
	default:
		g_assert_not_reached();
		/* fall-through just in case ;-) */

	case PNG_COLOR_TYPE_RGB_ALPHA:
		format = CAIRO_FORMAT_ARGB32;
		png_set_read_user_transform_fn (png, premultiply_data);
		break;

	case PNG_COLOR_TYPE_RGB:
		format = CAIRO_FORMAT_RGB24;
		png_set_read_user_transform_fn (png, convert_bytes_to_data);
		break;
	}

	stride = cairo_format_stride_for_width (format, png_width);
	if (stride < 0) {
		fill_png_error (error, image_uri, NULL);
		g_object_unref (stream);
		png_destroy_read_struct (&png, &info, NULL);
		return NULL;
	}

	image = g_slice_new0 (GXPSImage);
	image->res_x = png_get_x_pixels_per_meter (png, info) * METERS_PER_INCH;
	if (image->res_x == 0)
		image->res_x = 96;
	image->res_y = png_get_y_pixels_per_meter (png, info) * METERS_PER_INCH;
	if (image->res_y == 0)
		image->res_y = 96;

	data = g_malloc (png_height * stride);
	row_pointers = g_new (png_byte *, png_height);

	for (i = 0; i < png_height; i++)
		row_pointers[i] = &data[i * stride];

	png_read_image (png, row_pointers);
	png_read_end (png, info);
	png_destroy_read_struct (&png, &info, NULL);
	g_object_unref (stream);
	g_free (row_pointers);

	image->surface = cairo_image_surface_create_for_data (data, format,
							      png_width, png_height,
							      stride);
	if (cairo_surface_status (image->surface)) {
		fill_png_error (error, image_uri, NULL);
		gxps_image_free (image);
		g_free (data);
		return NULL;
	}

	status = cairo_surface_set_user_data (image->surface,
					      &image_data_cairo_key,
					      data,
					      (cairo_destroy_func_t) g_free);
	if (status) {
		fill_png_error (error, image_uri, NULL);
		gxps_image_free (image);
		g_free (data);
		return NULL;
	}

	return image;
#else
    return NULL;
#endif  /* HAVE_LIBPNG */
}