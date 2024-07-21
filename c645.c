gdk_pixbuf__tiff_image_load (FILE *f, GError **error)
{
	return gdk_pixbuf__tiff_image_load_real (f, NULL, error);
}