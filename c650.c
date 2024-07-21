gdk_pixbuf__gif_image_begin_load (ModulePreparedNotifyFunc prepare_func,
				  ModuleUpdatedNotifyFunc update_func,
				  gpointer user_data,
                                  GError **error)
{
	GifContext *context;

#ifdef IO_GIFDEBUG
	count = 0;
#endif
	context = new_context ();
        context->error = error;
	context->prepare_func = prepare_func;
	context->update_func = update_func;
	context->user_data = user_data;

	return (gpointer) context;
}