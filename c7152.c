e_ews_autodiscover_ws_url_sync (ESource *source,
				CamelEwsSettings *settings,
                                const gchar *email_address,
                                const gchar *password,
                                GCancellable *cancellable,
                                GError **error)
{
	EAsyncClosure *closure;
	GAsyncResult *result;
	gboolean success;

	g_return_val_if_fail (CAMEL_IS_EWS_SETTINGS (settings), FALSE);
	g_return_val_if_fail (email_address != NULL, FALSE);
	g_return_val_if_fail (password != NULL, FALSE);

	closure = e_async_closure_new ();

	e_ews_autodiscover_ws_url (source, settings, email_address, password, cancellable,
		e_async_closure_callback, closure);

	result = e_async_closure_wait (closure);

	success = e_ews_autodiscover_ws_url_finish (settings, result, error);

	e_async_closure_free (closure);

	return success;
}