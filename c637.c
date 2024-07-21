gdk_pixbuf__tiff_image_begin_load (ModulePreparedNotifyFunc prepare_func,
				   ModuleUpdatedNotifyFunc update_func,
				   gpointer user_data,
                                   GError **error)
{
	TiffData *context;
	gint fd;

	context = g_new (TiffData, 1);
	context->prepare_func = prepare_func;
	context->update_func = update_func;
	context->user_data = user_data;
	context->all_okay = TRUE;
	fd = g_file_open_tmp ("gdkpixbuf-tif-tmp.XXXXXX", &context->tempname,
			      NULL);
	if (fd < 0) {
		g_free (context);
		return NULL;
	}

	context->file = fdopen (fd, "w");
	if (context->file == NULL) {
                g_free (context->tempname);
		g_free (context);
		return NULL;
	}

	return context;
}