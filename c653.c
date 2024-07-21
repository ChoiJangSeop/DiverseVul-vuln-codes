new_context (void)
{
	GifContext *context;

	context = g_new0 (GifContext, 1);

        context->animation = g_object_new (GDK_TYPE_PIXBUF_GIF_ANIM, NULL);        
	context->frame = NULL;
	context->file = NULL;
	context->state = GIF_START;
	context->prepare_func = NULL;
	context->update_func = NULL;
	context->user_data = NULL;
	context->buf = NULL;
	context->amount_needed = 0;
	context->gif89.transparent = -1;
	context->gif89.delay_time = -1;
	context->gif89.input_flag = -1;
	context->gif89.disposal = -1;

	return context;
}