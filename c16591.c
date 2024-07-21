static GdkPixbuf *get_contiguous_pixbuf (guint width, 
					 guint height, 
					 gboolean has_alpha)
{
	guchar *pixels;
	guint channels, rowstride, bytes;
	
	if (has_alpha) 
		channels = 4;
	else 
		channels = 3;
	
	rowstride = width * channels;
	
	if (rowstride / channels != width)
                return NULL;                

	bytes = height * rowstride;

        if (bytes / rowstride != height)
                return NULL;                

        pixels = g_try_malloc (bytes);

	if (!pixels)
		return NULL;
	
	return gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, has_alpha, 8,
					 width, height, rowstride, free_buffer, NULL);
}