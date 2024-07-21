e_ews_connection_download_oal_file (EEwsConnection *cnc,
                                    const gchar *cache_filename,
                                    EwsProgressFn progress_fn,
                                    gpointer progress_data,
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
		e_ews_connection_download_oal_file);

	if (!soup_message) {
		g_simple_async_result_take_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		return;
	}

	data = g_slice_new0 (struct _oal_req_data);
	data->cnc = g_object_ref (cnc);
	data->soup_message = soup_message;  /* the session owns this */
	data->cache_filename = g_strdup (cache_filename);
	data->progress_fn = progress_fn;
	data->progress_data = progress_data;

	if (G_IS_CANCELLABLE (cancellable)) {
		data->cancellable = g_object_ref (cancellable);
		data->cancel_id = g_cancellable_connect (
			data->cancellable,
			G_CALLBACK (ews_cancel_msg),
			data, (GDestroyNotify) NULL);
	}

	g_simple_async_result_set_op_res_gpointer (
		simple, data, (GDestroyNotify) oal_req_data_free);

	/*
	 * Don't use streaming-based messages when we are loggin the traffic
	 * to generate trace files for tests
	 */
	if (e_ews_debug_get_log_level () <= 2)
		soup_message_body_set_accumulate (soup_message->response_body, FALSE);

	g_signal_connect (
		soup_message, "got-headers",
		G_CALLBACK (ews_soup_got_headers), data);
	g_signal_connect (
		soup_message, "got-chunk",
		G_CALLBACK (ews_soup_got_chunk), data);
	g_signal_connect (
		soup_message, "restarted",
		G_CALLBACK (ews_soup_restarted), data);

	ews_connection_schedule_queue_message (cnc, soup_message, oal_download_response_cb, simple);
}