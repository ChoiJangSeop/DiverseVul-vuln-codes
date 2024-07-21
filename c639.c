gdk_pixbuf__gif_image_load_animation (FILE *file,
                                      GError **error)
{
	GifContext *context;
	GdkPixbufAnimation *animation;

	g_return_val_if_fail (file != NULL, NULL);

	context = new_context ();

        context->error = error;
	context->file = file;

	if (gif_main_loop (context) == -1 || context->animation->frames == NULL) {
                if (context->error && *(context->error) == NULL)
                        g_set_error (context->error,
                                     GDK_PIXBUF_ERROR,
                                     GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
                                     _("GIF file was missing some data (perhaps it was truncated somehow?)"));

                g_object_unref (G_OBJECT (context->animation));
                context->animation = NULL;
        }

        if (context->animation)
                animation = GDK_PIXBUF_ANIMATION (context->animation);
        else
                animation = NULL;

        if (context->error && *(context->error))
                g_print ("%s\n", (*(context->error))->message);
        
        g_free (context->buf);
	g_free (context);
	return animation;
}