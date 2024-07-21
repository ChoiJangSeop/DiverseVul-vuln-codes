autodiscover_response_cb (SoupSession *session,
                          SoupMessage *msg,
                          gpointer data)

{
	GSimpleAsyncResult *simple = data;
	struct _autodiscover_data *ad;
	EwsUrls exch_urls, expr_urls;
	guint status = msg->status_code;
	xmlDoc *doc;
	xmlNode *node;
	gint idx;
	GError *error = NULL;

	memset (&exch_urls, 0, sizeof (EwsUrls));
	memset (&expr_urls, 0, sizeof (EwsUrls));

	ad = g_simple_async_result_get_op_res_gpointer (simple);

	for (idx = 0; idx < 5; idx++) {
		if (ad->msgs[idx] == msg)
			break;
	}
	if (idx == 5) {
		/* We already got removed (cancelled). Do nothing */
		goto unref;
	}

	ad->msgs[idx] = NULL;

	if (status != 200) {
		gboolean expired = FALSE;
		gchar *service_url = NULL;

		if (e_ews_connection_utils_check_x_ms_credential_headers (msg, NULL, &expired, &service_url) && expired) {
			e_ews_connection_utils_expired_password_to_error (service_url, &error);
		} else {
			g_set_error (
				&error, SOUP_HTTP_ERROR, status,
				"%d %s", status, msg->reason_phrase);
		}

		g_free (service_url);

		goto failed;
	}

	e_ews_debug_dump_raw_soup_response (msg);
	doc = xmlReadMemory (
		msg->response_body->data,
		msg->response_body->length,
		"autodiscover.xml", NULL, 0);
	if (!doc) {
		g_set_error (
			&error, EWS_CONNECTION_ERROR, -1,
			_("Failed to parse autodiscover response XML"));
		goto failed;
	}
	node = xmlDocGetRootElement (doc);
	if (strcmp ((gchar *) node->name, "Autodiscover")) {
		g_set_error (
			&error, EWS_CONNECTION_ERROR, -1,
			_("Failed to find <Autodiscover> element"));
		goto failed;
	}
	for (node = node->children; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE &&
		    !strcmp ((gchar *) node->name, "Response"))
			break;
	}
	if (!node) {
		g_set_error (
			&error, EWS_CONNECTION_ERROR, -1,
			_("Failed to find <Response> element"));
		goto failed;
	}
	for (node = node->children; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE &&
		    !strcmp ((gchar *) node->name, "Account"))
			break;
	}
	if (!node) {
		g_set_error (
			&error, EWS_CONNECTION_ERROR, -1,
			_("Failed to find <Account> element"));
		goto failed;
	}

	for (node = node->children; node; node = node->next) {
		if (node->type == XML_ELEMENT_NODE &&
		    !strcmp ((gchar *) node->name, "Protocol")) {
			xmlChar *protocol_type = autodiscover_get_protocol_type (node);

			if (g_strcmp0 ((const gchar *) protocol_type, "EXCH") == 0) {
				ews_urls_free_content (&exch_urls);
				autodiscover_parse_protocol (node, &exch_urls);
			} else if (g_strcmp0 ((const gchar *) protocol_type, "EXPR") == 0) {
				ews_urls_free_content (&expr_urls);
				autodiscover_parse_protocol (node, &expr_urls);

				/* EXPR has precedence, thus stop once found both there */
				if (expr_urls.as_url && expr_urls.oab_url) {
					xmlFree (protocol_type);
					break;
				}
			}

			if (protocol_type)
				xmlFree (protocol_type);
		}
	}

	/* Make the <OABUrl> optional */
	if (!exch_urls.as_url && !expr_urls.as_url) {
		ews_urls_free_content (&exch_urls);
		ews_urls_free_content (&expr_urls);
		g_set_error (
			&error, EWS_CONNECTION_ERROR, -1,
			_("Failed to find <ASUrl> in autodiscover response"));
		goto failed;
	}

	/* We have a good response; cancel all the others */
	for (idx = 0; idx < 5; idx++) {
		if (ad->msgs[idx]) {
			SoupMessage *m = ad->msgs[idx];
			ad->msgs[idx] = NULL;
			ews_connection_schedule_cancel_message (ad->cnc, m);
		}
	}

	if (expr_urls.as_url)
		ad->as_url = g_strdup ((gchar *) expr_urls.as_url);
	else if (exch_urls.as_url)
		ad->as_url = g_strdup ((gchar *) exch_urls.as_url);

	if (expr_urls.as_url && expr_urls.oab_url)
		ad->oab_url = g_strdup ((gchar *) expr_urls.oab_url);
	else if (!expr_urls.as_url && exch_urls.oab_url)
		ad->oab_url = g_strdup ((gchar *) exch_urls.oab_url);

	ews_urls_free_content (&exch_urls);
	ews_urls_free_content (&expr_urls);

	goto exit;

 failed:
	for (idx = 0; idx < 5; idx++) {
		if (ad->msgs[idx]) {
			/* There's another request outstanding.
			 * Hope that it has better luck. */
			g_clear_error (&error);
			goto unref;
		}
	}

	/* FIXME: We're actually returning the *last* error here,
	 * and in some cases (stupid firewalls causing timeouts)
	 * that's going to be the least interesting one. We probably
	 * want the *first* error */
	g_simple_async_result_take_error (simple, error);

 exit:
	g_simple_async_result_complete_in_idle (simple);

 unref:
	/* This function is processed within e_ews_soup_thread() and the 'simple'
	 * holds reference to EEwsConnection. For cases when this is the last
	 * reference to 'simple' the unref would cause crash, because of g_thread_join()
	 * in connection's dispose, trying to wait on the end of itself, thus it's
	 * safer to unref the 'simple' in a dedicated thread.
	*/
	e_ews_connection_utils_unref_in_thread (simple);
}