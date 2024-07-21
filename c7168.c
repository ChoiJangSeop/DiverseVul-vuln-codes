oal_response_cb (SoupSession *soup_session,
                 SoupMessage *soup_message,
                 gpointer user_data)
{
	GSimpleAsyncResult *simple;
	struct _oal_req_data *data;
	const gchar *etag;
	xmlDoc *doc;
	xmlNode *node;

	simple = G_SIMPLE_ASYNC_RESULT (user_data);
	data = g_simple_async_result_get_op_res_gpointer (simple);

	if (ews_connection_credentials_failed (data->cnc, soup_message, simple)) {
		goto exit;
	} else if (soup_message->status_code != 200) {
		if (soup_message->status_code == SOUP_STATUS_UNAUTHORIZED &&
		    soup_message->response_headers) {
			const gchar *diagnostics;

			diagnostics = soup_message_headers_get_list (soup_message->response_headers, "X-MS-DIAGNOSTICS");
			if (diagnostics && strstr (diagnostics, "invalid_grant")) {
				g_simple_async_result_set_error (
					simple,
					EWS_CONNECTION_ERROR,
					EWS_CONNECTION_ERROR_ACCESSDENIED,
					"%s", diagnostics);
				goto exit;
			} else if (diagnostics && *diagnostics) {
				g_simple_async_result_set_error (
					simple,
					EWS_CONNECTION_ERROR,
					EWS_CONNECTION_ERROR_AUTHENTICATION_FAILED,
					"%s", diagnostics);
				goto exit;
			}
		}
		g_simple_async_result_set_error (
			simple, SOUP_HTTP_ERROR,
			soup_message->status_code,
			"%d %s",
			soup_message->status_code,
			soup_message->reason_phrase);
		goto exit;
	}

	etag = soup_message_headers_get_one(soup_message->response_headers,
					    "ETag");
	if (etag)
		data->etag = g_strdup(etag);

	e_ews_debug_dump_raw_soup_response (soup_message);

	doc = xmlReadMemory (
		soup_message->response_body->data,
		soup_message->response_body->length,
		"oab.xml", NULL, 0);
	if (doc == NULL) {
		g_simple_async_result_set_error (
			simple, EWS_CONNECTION_ERROR, -1,
			"%s", _("Failed to parse oab XML"));
		goto exit;
	}

	node = xmlDocGetRootElement (doc);
	if (strcmp ((gchar *) node->name, "OAB") != 0) {
		g_simple_async_result_set_error (
			simple, EWS_CONNECTION_ERROR, -1,
			"%s", _("Failed to find <OAB> element\n"));
		goto exit_doc;
	}

	for (node = node->children; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE && strcmp ((gchar *) node->name, "OAL") == 0) {
			if (data->oal_id == NULL) {
				EwsOAL *oal = g_new0 (EwsOAL, 1);

				oal->id = get_property (node, "id");
				oal->dn = get_property (node, "dn");
				oal->name = get_property (node, "name");

				data->oals = g_slist_prepend (data->oals, oal);
			} else {
				gchar *id = get_property (node, "id");

				if (strcmp (id, data->oal_id) == 0) {
					/* parse details of full_details file */
					data->elements = parse_oal_full_details (node, data->oal_element);

					g_free (id);
					break;
				}

				g_free (id);
			}
		}
	}

	data->oals = g_slist_reverse (data->oals);

 exit_doc:
	xmlFreeDoc (doc);
 exit:
	g_simple_async_result_complete_in_idle (simple);
	/* This is run in cnc->priv->soup_thread, and the cnc is held by simple, thus
	 * for cases when the complete_in_idle is finished before the unref call, when
	 * the cnc will be left with the last reference and thus cannot join the soup_thread
	 * while still in it, the unref is done in a dedicated thread. */
	e_ews_connection_utils_unref_in_thread (simple);
}