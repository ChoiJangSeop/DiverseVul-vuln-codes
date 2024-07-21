gdk_pixbuf__tiff_image_load_increment (gpointer data, const guchar *buf,
                                       guint size, GError **error)
{
	TiffData *context = (TiffData *) data;

	g_return_val_if_fail (data != NULL, FALSE);

	if (fwrite (buf, sizeof (guchar), size, context->file) != size) {
		context->all_okay = FALSE;
                g_set_error (error,
                             G_FILE_ERROR,
                             g_file_error_from_errno (errno),
                             _("Failed to write to temporary file when loading TIFF image"));
		return FALSE;
	}

	return TRUE;
}