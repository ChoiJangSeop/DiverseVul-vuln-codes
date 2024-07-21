ews_backend_ref_connection_thread (GSimpleAsyncResult *simple,
                                   GObject *object,
                                   GCancellable *cancellable)
{
	EEwsConnection *connection;
	GError *error = NULL;

	connection = e_ews_backend_ref_connection_sync (E_EWS_BACKEND (object), NULL, cancellable, &error);

	/* Sanity check. */
	g_return_if_fail (
		((connection != NULL) && (error == NULL)) ||
		((connection == NULL) && (error != NULL)));

	if (connection != NULL)
		g_simple_async_result_set_op_res_gpointer (
			simple, connection, g_object_unref);

	if (error != NULL)
		g_simple_async_result_take_error (simple, error);
}