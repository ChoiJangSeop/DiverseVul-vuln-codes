gxps_images_get_image (GXPSArchive *zip,
		       const gchar *image_uri,
		       GError     **error)
{
	GXPSImage *image = NULL;
	gchar *image_uri_lower;

	/* First try with extensions,
	 * as it's recommended by the spec
	 * (2.1.5 Image Parts)
	 */
	image_uri_lower = g_utf8_strdown (image_uri, -1);
	if (g_str_has_suffix (image_uri_lower, ".png")) {
		image = gxps_images_create_from_png (zip, image_uri, error);
	} else if (g_str_has_suffix (image_uri_lower, ".jpg")) {
		image = gxps_images_create_from_jpeg (zip, image_uri, error);
	} else if (g_str_has_suffix (image_uri_lower, ".tif")) {
		image = gxps_images_create_from_tiff (zip, image_uri, error);
	} else if (g_str_has_suffix (image_uri_lower, "wdp")) {
		GXPS_DEBUG (g_message ("Unsupported image format windows media photo"));
		g_free (image_uri_lower);
		return NULL;
	}

	g_free (image_uri_lower);

	if (!image) {
		gchar *mime_type;

		mime_type = gxps_images_guess_content_type (zip, image_uri);
		if (g_strcmp0 (mime_type, "image/png") == 0) {
			image = gxps_images_create_from_png (zip, image_uri, error);
		} else if (g_strcmp0 (mime_type, "image/jpeg") == 0) {
			image = gxps_images_create_from_jpeg (zip, image_uri, error);
		} else if (g_strcmp0 (mime_type, "image/tiff") == 0) {
			image = gxps_images_create_from_tiff (zip, image_uri, error);
		} else {
			GXPS_DEBUG (g_message ("Unsupported image format: %s", mime_type));
		}
		g_free (mime_type);
	}

	return image;
}