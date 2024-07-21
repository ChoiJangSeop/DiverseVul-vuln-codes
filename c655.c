gdk_pixbuf__tiff_image_stop_load (gpointer data,
                                  GError **error)
{
	TiffData *context = (TiffData*) data;
        gboolean retval = FALSE;
        
	g_return_val_if_fail (data != NULL, TRUE);

	fflush (context->file);
	rewind (context->file);
	if (context->all_okay) {
                GdkPixbuf *pixbuf;
		pixbuf = gdk_pixbuf__tiff_image_load_real (context->file, context, error);
                if (pixbuf != NULL)
                        retval = TRUE;
        }

	fclose (context->file);
	unlink (context->tempname);
	g_free (context->tempname);
	g_free ((TiffData *) context);

        return TRUE;
}