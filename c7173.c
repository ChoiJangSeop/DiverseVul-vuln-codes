e_ews_connection_get_oal_detail (EEwsConnection *cnc,
                                 const gchar *oal_id,
                                 const gchar *oal_element,
				 const gchar *etag,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data)
{
	GSimpleAsyncResult *simple;
	SoupMessage *soup_message;
	struct _oal_req_data *data;
	gchar *sep;
	GError *error = NULL;

	g_return_if_fail (E_IS_EWS_CONNECTION (cnc));

	soup_message = e_ews_get_msg_for_url (cnc->priv->settings, cnc->priv->uri, NULL, &error);

	simple = g_simple_async_result_new (
		G_OBJECT (cnc), callback, user_data,
		e_ews_connection_get_oal_detail);

	if (!soup_message) {
		g_simple_async_result_take_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
		return;
	}

	if (etag && *etag)
		soup_message_headers_append (soup_message->request_headers,
					     "If-None-Match", etag);

	data = g_slice_new0 (struct _oal_req_data);
	data->cnc = g_object_ref (cnc);
	data->soup_message = soup_message;  /* the session owns this */
	data->oal_id = g_strdup (oal_id);
	data->oal_element = g_strdup (oal_element);

	/* oal_id can be of form "GUID:name", but here is compared only GUID */
	sep = strchr (data->oal_id, ':');
	if (sep)
		*sep = '\0';

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