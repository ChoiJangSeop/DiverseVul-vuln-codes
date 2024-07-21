e_ews_connection_get_oal_list (EEwsConnection *cnc,
                               GCancellable *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer user_data)
{
	GSimpleAsyncResult *simple;
	SoupMessage *soup_message;
	struct _oal_req_data *data;
	GError *error = NULL;

	g_return_if_fail (E_IS_EWS_CONNECTION (cnc));

	soup_message = e_ews_get_msg_for_url (cnc->priv->settings, cnc->priv->uri, NULL, &error);

	simple = g_simple_async_result_new (
		G_OBJECT (cnc), callback, user_data,
		e_ews_connection_get_oal_list);

	if (!soup_message) {
		g_simple_async_result_take_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		return;
	}

	data = g_slice_new0 (struct _oal_req_data);
	data->cnc = g_object_ref (cnc);
	data->soup_message = soup_message;  /* the session owns this */

	if (G_IS_CANCELLABLE (cancellable)) {
		data->cancellable = g_object_ref (cancellable);
		data->cancel_id = g_cancellable_connect (
			data->cancellable,
			G_CALLBACK (ews_cancel_msg),
			data, (GDestroyNotify) NULL);
	}

	g_simple_async_result_set_op_res_gpointer (
		simple, data, (GDestroyNotify) oal_req_data_free);

	ews_connection_schedule_queue_message (cnc, soup_message, oal_response_cb, simple);
}