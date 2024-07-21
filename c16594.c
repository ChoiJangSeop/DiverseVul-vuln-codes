gdk_pixbuf_new (GdkColorspace colorspace, 
                gboolean      has_alpha,
                int           bits_per_sample,
                int           width,
                int           height)
{
	guchar *buf;
	int channels;
	int rowstride;
        gsize bytes;

	g_return_val_if_fail (colorspace == GDK_COLORSPACE_RGB, NULL);
	g_return_val_if_fail (bits_per_sample == 8, NULL);
	g_return_val_if_fail (width > 0, NULL);
	g_return_val_if_fail (height > 0, NULL);

	channels = has_alpha ? 4 : 3;
        rowstride = width * channels;
        if (rowstride / channels != width || rowstride + 3 < 0) /* overflow */
                return NULL;
        
	/* Always align rows to 32-bit boundaries */
	rowstride = (rowstride + 3) & ~3;

        bytes = height * rowstride;
        if (bytes / rowstride !=  height) /* overflow */
                return NULL;
            
	buf = g_try_malloc (bytes);
	if (!buf)
		return NULL;

	return gdk_pixbuf_new_from_data (buf, colorspace, has_alpha, bits_per_sample,
					 width, height, rowstride,
					 free_buffer, NULL);
}