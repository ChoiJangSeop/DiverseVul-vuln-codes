gdk_pixbuf__png_image_save (FILE          *f, 
                            GdkPixbuf     *pixbuf, 
                            gchar        **keys,
                            gchar        **values,
                            GError       **error)
{
       png_structp png_ptr;
       png_infop info_ptr;
       guchar *ptr;
       guchar *pixels;
       int x, y, j;
       png_bytep row_ptr, data = NULL;
       png_color_8 sig_bit;
       int w, h, rowstride;
       int has_alpha;
       int bpc;

       if (keys && *keys) {
               g_warning ("Bad option name '%s' passed to PNG saver",
                          *keys);
               return FALSE;
#if 0
               gchar **kiter = keys;
               gchar **viter = values;

               
               while (*kiter) {
                       
                       ++kiter;
                       ++viter;
               }
#endif
       }
       
       bpc = gdk_pixbuf_get_bits_per_sample (pixbuf);
       w = gdk_pixbuf_get_width (pixbuf);
       h = gdk_pixbuf_get_height (pixbuf);
       rowstride = gdk_pixbuf_get_rowstride (pixbuf);
       has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
       pixels = gdk_pixbuf_get_pixels (pixbuf);

       png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
                                          error,
                                          png_simple_error_callback,
                                          png_simple_warning_callback);

       g_return_val_if_fail (png_ptr != NULL, FALSE);

       info_ptr = png_create_info_struct (png_ptr);
       if (info_ptr == NULL) {
               png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
               return FALSE;
       }
       if (setjmp (png_ptr->jmpbuf)) {
               png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
               return FALSE;
       }
       png_init_io (png_ptr, f);
       if (has_alpha) {
               png_set_IHDR (png_ptr, info_ptr, w, h, bpc,
                             PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
#ifdef WORDS_BIGENDIAN
               png_set_swap_alpha (png_ptr);
#else
               png_set_bgr (png_ptr);
#endif
       } else {
               png_set_IHDR (png_ptr, info_ptr, w, h, bpc,
                             PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
               data = g_try_malloc (w * 3 * sizeof (char));

               if (data == NULL) {
                       /* Check error NULL, normally this would be broken,
                        * but libpng makes me want to code defensively.
                        */
                       if (error && *error == NULL) {
                               g_set_error (error,
                                            GDK_PIXBUF_ERROR,
                                            GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                            _("Insufficient memory to save PNG file"));
                       }
                       png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
                       return FALSE;
               }
       }
       sig_bit.red = bpc;
       sig_bit.green = bpc;
       sig_bit.blue = bpc;
       sig_bit.alpha = bpc;
       png_set_sBIT (png_ptr, info_ptr, &sig_bit);
       png_write_info (png_ptr, info_ptr);
       png_set_shift (png_ptr, &sig_bit);
       png_set_packing (png_ptr);

       ptr = pixels;
       for (y = 0; y < h; y++) {
               if (has_alpha)
                       row_ptr = (png_bytep)ptr;
               else {
                       for (j = 0, x = 0; x < w; x++)
                               memcpy (&(data[x*3]), &(ptr[x*3]), 3);

                       row_ptr = (png_bytep)data;
               }
               png_write_rows (png_ptr, &row_ptr, 1);
               ptr += rowstride;
       }

       if (data)
               g_free (data);

       png_write_end (png_ptr, info_ptr);
       png_destroy_write_struct (&png_ptr, (png_infopp) NULL);

       return TRUE;
}