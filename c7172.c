e_ews_autodiscover_ws_url (ESource *source,
			   CamelEwsSettings *settings,
                           const gchar *email_address,
                           const gchar *password,
                           GCancellable *cancellable,
                           GAsyncReadyCallback callback,
                           gpointer user_data)
{
	GSimpleAsyncResult *simple;
	struct _autodiscover_data *ad;
	xmlOutputBuffer *buf;
	gchar *url1, *url2, *url3, *url4, *url5;
	gchar *domain;
	xmlDoc *doc;
	EEwsConnection *cnc;
	SoupURI *soup_uri = NULL;
	gboolean use_secure = TRUE;
	const gchar *host_url;
	GError *error = NULL;

	g_return_if_fail (CAMEL_IS_EWS_SETTINGS (settings));
	g_return_if_fail (email_address != NULL);
	g_return_if_fail (password != NULL);

	simple = g_simple_async_result_new (
		G_OBJECT (settings), callback,
		user_data, e_ews_autodiscover_ws_url);

	domain = strchr (email_address, '@');
	if (domain == NULL || *domain == '\0') {
		g_simple_async_result_set_error (
			simple, EWS_CONNECTION_ERROR, -1,
			"%s", _("Email address is missing a domain part"));
		g_simple_async_result_complete_in_idle (simple);
		g_object_unref (simple);
		return;
	}
	domain++;

	doc = e_ews_autodiscover_ws_xml (email_address);
	buf = xmlAllocOutputBuffer (NULL);
	xmlNodeDumpOutput (buf, doc, xmlDocGetRootElement (doc), 0, 1, NULL);
	xmlOutputBufferFlush (buf);

	url1 = NULL;
	url2 = NULL;
	url3 = NULL;
	url4 = NULL;
	url5 = NULL;

	host_url = camel_ews_settings_get_hosturl (settings);
	if (host_url != NULL)
		soup_uri = soup_uri_new (host_url);

	if (soup_uri != NULL) {
		const gchar *host = soup_uri_get_host (soup_uri);
		const gchar *scheme = soup_uri_get_scheme (soup_uri);

		use_secure = g_strcmp0 (scheme, "https") == 0;

		url1 = g_strdup_printf ("http%s://%s/autodiscover/autodiscover.xml", use_secure ? "s" : "", host);
		url2 = g_strdup_printf ("http%s://autodiscover.%s/autodiscover/autodiscover.xml", use_secure ? "s" : "", host);

		/* outlook.office365.com has its autodiscovery at outlook.com */
		if (host && g_ascii_strcasecmp (host, "outlook.office365.com") == 0 &&
		   domain && g_ascii_strcasecmp (host, "outlook.com") != 0) {
			url5 = g_strdup_printf ("https://outlook.com/autodiscover/autodiscover.xml");
		}

		soup_uri_free (soup_uri);
	}

	url3 = g_strdup_printf ("http%s://%s/autodiscover/autodiscover.xml", use_secure ? "s" : "", domain);
	url4 = g_strdup_printf ("http%s://autodiscover.%s/autodiscover/autodiscover.xml", use_secure ? "s" : "", domain);

	cnc = e_ews_connection_new (source, url3, settings);
	e_ews_connection_set_password (cnc, password);

	/*
	 * http://msdn.microsoft.com/en-us/library/ee332364.aspx says we are
	 * supposed to try $domain and then autodiscover.$domain. But some
	 * people have broken firewalls on the former which drop packets
	 * instead of rejecting connections, and make the request take ages
	 * to time out. So run both queries in parallel and let the fastest
	 * (successful) one win.
	 */
	ad = g_slice_new0 (struct _autodiscover_data);
	ad->cnc = cnc;  /* takes ownership */
	ad->buf = buf;  /* takes ownership */

	if (G_IS_CANCELLABLE (cancellable)) {
		ad->cancellable = g_object_ref (cancellable);
		ad->cancel_id = g_cancellable_connect (
			ad->cancellable,
			G_CALLBACK (autodiscover_cancelled_cb),
			g_object_ref (cnc),
			g_object_unref);
	}

	g_simple_async_result_set_op_res_gpointer (
		simple, ad, (GDestroyNotify) autodiscover_data_free);

	/* Passing a NULL URL string returns NULL. */
	ad->msgs[0] = e_ews_get_msg_for_url (settings, url1, buf, &error);
	ad->msgs[1] = e_ews_get_msg_for_url (settings, url2, buf, NULL);
	ad->msgs[2] = e_ews_get_msg_for_url (settings, url3, buf, NULL);
	ad->msgs[3] = e_ews_get_msg_for_url (settings, url4, buf, NULL);
	ad->msgs[4] = e_ews_get_msg_for_url (settings, url5, buf, NULL);

	/* These have to be submitted only after they're both set in ad->msgs[]
	 * or there will be races with fast completion */
	if (ad->msgs[0] != NULL)
		ews_connection_schedule_queue_message (cnc, ad->msgs[0], autodiscover_response_cb, g_object_ref (simple));
	if (ad->msgs[1] != NULL)
		ews_connection_schedule_queue_message (cnc, ad->msgs[1], autodiscover_response_cb, g_object_ref (simple));
	if (ad->msgs[2] != NULL)
		ews_connection_schedule_queue_message (cnc, ad->msgs[2], autodiscover_response_cb, g_object_ref (simple));
	if (ad->msgs[3] != NULL)
		ews_connection_schedule_queue_message (cnc, ad->msgs[3], autodiscover_response_cb, g_object_ref (simple));
	if (ad->msgs[4] != NULL)
		ews_connection_schedule_queue_message (cnc, ad->msgs[4], autodiscover_response_cb, g_object_ref (simple));

	xmlFreeDoc (doc);
	g_free (url1);
	g_free (url2);
	g_free (url3);
	g_free (url4);

	if (error && !ad->msgs[0] && !ad->msgs[1] && !ad->msgs[2] && !ad->msgs[3] && !ad->msgs[4]) {
		g_simple_async_result_take_error (simple, error);
		g_simple_async_result_complete_in_idle (simple);
	} else {
		g_clear_error (&error);

		/* each request holds a reference to 'simple',
		 * thus remove one, to have it actually freed */
		g_object_unref (simple);
	}
}