oal_download_response_cb (SoupSession *soup_session,
                          SoupMessage *soup_message,
                          gpointer user_data)
{
	GSimpleAsyncResult *simple;
	struct _oal_req_data *data;

	simple = G_SIMPLE_ASYNC_RESULT (user_data);
	data = g_simple_async_result_get_op_res_gpointer (simple);

	if (ews_connection_credentials_failed (data->cnc, soup_message, simple)) {
		g_unlink (data->cache_filename);
	} else if (soup_message->status_code != 200) {
		g_simple_async_result_set_error (
			simple, SOUP_HTTP_ERROR,
			soup_message->status_code,
			"%d %s",
			soup_message->status_code,
			soup_message->reason_phrase);
		g_unlink (data->cache_filename);

	} else if (data->error != NULL) {
		g_simple_async_result_take_error (simple, data->error);
		data->error = NULL;
		g_unlink (data->cache_filename);
	}

	e_ews_debug_dump_raw_soup_response (soup_message);

	g_simple_async_result_complete_in_idle (simple);
	e_ews_connection_utils_unref_in_thread (simple);
}