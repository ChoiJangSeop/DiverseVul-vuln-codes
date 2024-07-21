gdk_pixbuf_copy (const GdkPixbuf *pixbuf)
{
	guchar *buf;
	int size;

	g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

	/* Calculate a semi-exact size.  Here we copy with full rowstrides;
	 * maybe we should copy each row individually with the minimum
	 * rowstride?
	 */

	size = gdk_pixbuf_get_byte_length (pixbuf);

	buf = g_try_malloc (size * sizeof (guchar));
	if (!buf)
		return NULL;

	memcpy (buf, pixbuf->pixels, size);

	return gdk_pixbuf_new_from_data (buf,
					 pixbuf->colorspace, pixbuf->has_alpha,
					 pixbuf->bits_per_sample,
					 pixbuf->width, pixbuf->height,
					 pixbuf->rowstride,
					 free_buffer,
					 NULL);
}