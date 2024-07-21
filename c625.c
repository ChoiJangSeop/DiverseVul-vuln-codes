gdk_pixbuf__gif_image_load (FILE *file, GError **error)
{
	GifContext *context;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (file != NULL, NULL);

	context = new_context ();

        if (context == NULL) {
                g_set_error_literal (error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                     _("Not enough memory to load GIF file"));
                return NULL;
        }
        
	context->file = file;
        context->error = error;
        context->stop_after_first_frame = TRUE;

	if (gif_main_loop (context) == -1 || context->animation->frames == NULL) {
                if (context->error && *(context->error) == NULL)
                        g_set_error_literal (context->error,
                                             GDK_PIXBUF_ERROR,
                                             GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                             _("GIF file was missing some data (perhaps it was truncated somehow?)"));
        }
        
        pixbuf = gdk_pixbuf_animation_get_static_image (GDK_PIXBUF_ANIMATION (context->animation));

        if (pixbuf)
                g_object_ref (pixbuf);

        g_object_unref (context->animation);
        
        g_free (context->buf);
	g_free (context);
 
	return pixbuf;
}