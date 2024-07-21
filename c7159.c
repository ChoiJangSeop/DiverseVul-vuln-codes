ews_response_cb (SoupSession *session,
                 SoupMessage *msg,
                 gpointer data)
{
	EwsNode *enode = (EwsNode *) data;
	ESoapResponse *response;
	ESoapParameter *param;
	const gchar *persistent_auth;
	gint log_level;
	gint wait_ms = 0;

	persistent_auth = soup_message_headers_get_one (msg->response_headers, "Persistent-Auth");
	if (persistent_auth && g_ascii_strcasecmp (persistent_auth, "false") == 0) {
		SoupSessionFeature *feature;

		feature = soup_session_get_feature (session, SOUP_TYPE_AUTH_MANAGER);
		if (feature) {
			soup_auth_manager_clear_cached_credentials (SOUP_AUTH_MANAGER (feature));
		}
	}

	if (g_cancellable_is_cancelled (enode->cancellable))
		goto exit;

	if (ews_connection_credentials_failed (enode->cnc, msg, enode->simple)) {
		goto exit;
	} else if (msg->status_code == SOUP_STATUS_UNAUTHORIZED) {
		if (msg->response_headers) {
			const gchar *diagnostics;

			diagnostics = soup_message_headers_get_list (msg->response_headers, "X-MS-DIAGNOSTICS");
			if (diagnostics && strstr (diagnostics, "invalid_grant")) {
				g_simple_async_result_set_error (
					enode->simple,
					EWS_CONNECTION_ERROR,
					EWS_CONNECTION_ERROR_ACCESSDENIED,
					"%s", diagnostics);
				goto exit;
			} else if (diagnostics && *diagnostics) {
				g_simple_async_result_set_error (
					enode->simple,
					EWS_CONNECTION_ERROR,
					EWS_CONNECTION_ERROR_AUTHENTICATION_FAILED,
					"%s", diagnostics);
				goto exit;
			}
		}

		g_simple_async_result_set_error (
			enode->simple,
			EWS_CONNECTION_ERROR,
			EWS_CONNECTION_ERROR_AUTHENTICATION_FAILED,
			_("Authentication failed"));
		goto exit;
	} else if (msg->status_code == SOUP_STATUS_CANT_RESOLVE ||
		   msg->status_code == SOUP_STATUS_CANT_RESOLVE_PROXY ||
		   msg->status_code == SOUP_STATUS_CANT_CONNECT ||
		   msg->status_code == SOUP_STATUS_CANT_CONNECT_PROXY ||
		   msg->status_code == SOUP_STATUS_IO_ERROR) {
		g_simple_async_result_set_error (
			enode->simple,
			EWS_CONNECTION_ERROR,
			EWS_CONNECTION_ERROR_UNAVAILABLE,
			"%s", msg->reason_phrase);
		goto exit;
	}

	response = e_soap_message_parse_response ((ESoapMessage *) msg);

	if (response == NULL) {
		g_simple_async_result_set_error (
			enode->simple,
			EWS_CONNECTION_ERROR,
			EWS_CONNECTION_ERROR_NORESPONSE,
			_("No response: %s"), msg->reason_phrase);
		goto exit;
	}

	/* TODO: The stdout can be replaced with Evolution's
	 * Logging framework also */

	log_level = e_ews_debug_get_log_level ();
	if (log_level >= 1 && log_level < 3) {
		/* This will dump only the headers, since we stole the body.
		 * And only if EWS_DEBUG=1, since higher levels will have dumped
		 * it directly from libsoup anyway. */
		e_ews_debug_dump_raw_soup_response (msg);
		/* And this will dump the body... */
		e_soap_response_dump_response (response, stdout);
	}

	param = e_soap_response_get_first_parameter_by_name (response, "detail", NULL);
	if (param)
		param = e_soap_parameter_get_first_child_by_name (param, "ResponseCode");
	if (param) {
		gchar *value;

		value = e_soap_parameter_get_string_value (param);
		if (value && ews_get_error_code (value) == EWS_CONNECTION_ERROR_SERVERBUSY) {
			param = e_soap_response_get_first_parameter_by_name (response, "detail", NULL);
			if (param)
				param = e_soap_parameter_get_first_child_by_name (param, "MessageXml");
			if (param) {
				param = e_soap_parameter_get_first_child_by_name (param, "Value");
				if (param) {
					g_free (value);

					value = e_soap_parameter_get_property (param, "Name");
					if (g_strcmp0 (value, "BackOffMilliseconds") == 0) {
						wait_ms = e_soap_parameter_get_int_value (param);
					}
				}
			}
		}

		g_free (value);
	}

	if (wait_ms > 0 && e_ews_connection_get_backoff_enabled (enode->cnc)) {
		GCancellable *cancellable = enode->cancellable;
		EFlag *flag;

		if (cancellable)
			g_object_ref (cancellable);
		g_object_ref (msg);

		flag = e_flag_new ();
		while (wait_ms > 0 && !g_cancellable_is_cancelled (cancellable) && msg->status_code != SOUP_STATUS_CANCELLED) {
			gint64 now = g_get_monotonic_time ();
			gint left_minutes, left_seconds;

			left_minutes = wait_ms / 60000;
			left_seconds = (wait_ms / 1000) % 60;

			if (left_minutes > 0) {
				camel_operation_push_message (cancellable,
					g_dngettext (GETTEXT_PACKAGE,
						"Exchange server is busy, waiting to retry (%d:%02d minute)",
						"Exchange server is busy, waiting to retry (%d:%02d minutes)", left_minutes),
					left_minutes, left_seconds);
			} else {
				camel_operation_push_message (cancellable,
					g_dngettext (GETTEXT_PACKAGE,
						"Exchange server is busy, waiting to retry (%d second)",
						"Exchange server is busy, waiting to retry (%d seconds)", left_seconds),
					left_seconds);
			}

			e_flag_wait_until (flag, now + (G_TIME_SPAN_MILLISECOND * (wait_ms > 1000 ? 1000 : wait_ms)));

			now = g_get_monotonic_time () - now;
			now = now / G_TIME_SPAN_MILLISECOND;

			if (now >= wait_ms)
				wait_ms = 0;
			wait_ms -= now;

			camel_operation_pop_message (cancellable);
		}

		e_flag_free (flag);

		g_object_unref (response);

		if (g_cancellable_is_cancelled (cancellable) ||
		    msg->status_code == SOUP_STATUS_CANCELLED) {
			g_clear_object (&cancellable);
			g_object_unref (msg);
		} else {
			EwsNode *new_node;

			new_node = ews_node_new ();
			new_node->msg = E_SOAP_MESSAGE (msg); /* takes ownership */
			new_node->pri = enode->pri;
			new_node->cb = enode->cb;
			new_node->cnc = enode->cnc;
			new_node->simple = enode->simple;

			enode->simple = NULL;

			QUEUE_LOCK (enode->cnc);
			enode->cnc->priv->jobs = g_slist_prepend (enode->cnc->priv->jobs, new_node);
			QUEUE_UNLOCK (enode->cnc);

			if (cancellable) {
				new_node->cancellable = g_object_ref (cancellable);
				new_node->cancel_handler_id = g_cancellable_connect (
					cancellable, G_CALLBACK (ews_cancel_request), new_node, NULL);
			}

			g_clear_object (&cancellable);
		}

		goto exit;
	}

	if (enode->cb != NULL)
		enode->cb (response, enode->simple);

	g_object_unref (response);

exit:
	if (enode->simple)
		g_simple_async_result_complete_in_idle (enode->simple);

	ews_active_job_done (enode->cnc, enode);
}