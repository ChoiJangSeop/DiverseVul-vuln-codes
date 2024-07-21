async_context_free (gpointer ptr)
{
	AsyncContext *async_context = ptr;

	if (!async_context)
		return;

	g_clear_object (&async_context->autodiscover);
	g_clear_object (&async_context->activity);
	g_clear_object (&async_context->source);
	g_clear_object (&async_context->ews_settings);
	g_free (async_context->email_address);

	g_slice_free (AsyncContext, async_context);
}