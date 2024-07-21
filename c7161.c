e_ews_get_msg_for_url (CamelEwsSettings *settings,
		       const gchar *url,
                       xmlOutputBuffer *buf,
                       GError **error)
{
	SoupMessage *msg;

	if (url == NULL) {
		g_set_error_literal (
			error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
			_("URL cannot be NULL"));
		return NULL;
	}

	msg = soup_message_new (buf != NULL ? "POST" : "GET", url);
	if (!msg) {
		g_set_error (
			error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
			_("URL “%s” is not valid"), url);
		return NULL;
	}

	e_ews_message_attach_chunk_allocator (msg);

	e_ews_message_set_user_agent_header (msg, settings);

	if (buf != NULL) {
		soup_message_set_request (
			msg, "text/xml; charset=utf-8", SOUP_MEMORY_COPY,
			(gchar *)
			#ifdef LIBXML2_NEW_BUFFER
			xmlOutputBufferGetContent (buf), xmlOutputBufferGetSize (buf)
			#else
			buf->buffer->content, buf->buffer->use
			#endif
			);
		g_signal_connect (
			msg, "restarted",
			G_CALLBACK (post_restarted), buf);
	}

	e_ews_debug_dump_raw_soup_request (msg);

	return msg;
}