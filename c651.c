gdk_pixbuf__gif_image_load (FILE *file, GError **error)
{
	GifContext *context;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (file != NULL, NULL);

	context = new_context ();
	context->file = file;
        context->error = error;
        
	if (gif_main_loop (context) == -1 || context->animation->frames == NULL) {
                if (context->error && *(context->error) == NULL)
                        g_set_error (context->error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("GIF file was missing some data (perhaps it was truncated somehow?)"));
        }
        
        pixbuf = gdk_pixbuf_animation_get_static_image (GDK_PIXBUF_ANIMATION (context->animation));

        if (pixbuf)
                g_object_ref (G_OBJECT (pixbuf));

        g_object_unref (G_OBJECT (context->animation));
        
        g_free (context->buf);
	g_free (context);
 
	return pixbuf;
}