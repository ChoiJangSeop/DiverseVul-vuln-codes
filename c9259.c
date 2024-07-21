connect_to_server (CamelService *service,
                   GCancellable *cancellable,
                   GError **error)
{
	CamelPOP3Store *store = CAMEL_POP3_STORE (service);
	CamelNetworkSettings *network_settings;
	CamelNetworkSecurityMethod method;
	CamelSettings *settings;
	CamelStream *stream = NULL;
	CamelPOP3Engine *pop3_engine = NULL;
	CamelPOP3Command *pc;
	GIOStream *base_stream;
	GIOStream *tls_stream;
	gboolean disable_extensions;
	gboolean success = TRUE;
	gchar *host;
	guint32 flags = 0;
	gint ret;
	GError *local_error = NULL;

	settings = camel_service_ref_settings (service);

	network_settings = CAMEL_NETWORK_SETTINGS (settings);
	host = camel_network_settings_dup_host (network_settings);
	method = camel_network_settings_get_security_method (network_settings);

	disable_extensions = camel_pop3_settings_get_disable_extensions (
		CAMEL_POP3_SETTINGS (settings));

	g_object_unref (settings);

	base_stream = camel_network_service_connect_sync (
		CAMEL_NETWORK_SERVICE (service), cancellable, error);

	if (base_stream != NULL) {
		stream = camel_stream_new (base_stream);
		g_object_unref (base_stream);
	} else {
		success = FALSE;
		goto exit;
	}

	/* parent class connect initialization */
	if (CAMEL_SERVICE_CLASS (camel_pop3_store_parent_class)->
		connect_sync (service, cancellable, error) == FALSE) {
		g_object_unref (stream);
		success = FALSE;
		goto exit;
	}

	if (disable_extensions)
		flags |= CAMEL_POP3_ENGINE_DISABLE_EXTENSIONS;

	if (!(pop3_engine = camel_pop3_engine_new (stream, flags, cancellable, &local_error)) ||
	    local_error != NULL) {
		if (local_error)
			g_propagate_error (error, local_error);
		else
			g_set_error (
				error, CAMEL_ERROR, CAMEL_ERROR_GENERIC,
				_("Failed to read a valid greeting from POP server %s"),
				host);
		g_object_unref (stream);
		success = FALSE;
		goto exit;
	}

	if (method != CAMEL_NETWORK_SECURITY_METHOD_STARTTLS_ON_STANDARD_PORT) {
		g_object_unref (stream);
		goto exit;
	}

	if (!(pop3_engine->capa & CAMEL_POP3_CAP_STLS)) {
		g_set_error (
			error, CAMEL_ERROR, CAMEL_ERROR_GENERIC,
			_("Failed to connect to POP server %s in secure mode: %s"),
			host, _("STLS not supported by server"));
		goto stls_exception;
	}

	pc = camel_pop3_engine_command_new (
		pop3_engine, 0, NULL, NULL,
		cancellable, error, "STLS\r\n");
	while (camel_pop3_engine_iterate (pop3_engine, NULL, cancellable, NULL) > 0)
		;

	ret = pc->state == CAMEL_POP3_COMMAND_OK;
	camel_pop3_engine_command_free (pop3_engine, pc);

	if (ret == FALSE) {
		gchar *tmp;

		tmp = get_valid_utf8_error ((gchar *) pop3_engine->line);
		g_set_error (
			error, CAMEL_ERROR, CAMEL_ERROR_GENERIC,
			/* Translators: Last %s is an optional
			 * explanation beginning with ": " separator. */
			_("Failed to connect to POP server %s in secure mode%s"),
			host, (tmp != NULL) ? tmp : "");
		g_free (tmp);
		goto stls_exception;
	}

	/* Okay, now toggle SSL/TLS mode */
	base_stream = camel_stream_ref_base_stream (stream);
	tls_stream = camel_network_service_starttls (
		CAMEL_NETWORK_SERVICE (service), base_stream, error);
	g_object_unref (base_stream);

	if (tls_stream != NULL) {
		camel_stream_set_base_stream (stream, tls_stream);
		g_object_unref (tls_stream);
	} else {
		g_prefix_error (
			error,
			_("Failed to connect to POP server %s in secure mode: "),
			host);
		goto stls_exception;
	}

	g_clear_object (&stream);

	/* rfc2595, section 4 states that after a successful STLS
	 * command, the client MUST discard prior CAPA responses */
	if (!camel_pop3_engine_reget_capabilities (pop3_engine, cancellable, error))
		goto exception;

	goto exit;

stls_exception:
	/* As soon as we send a STLS command, all hope
	 * is lost of a clean QUIT if problems arise. */
	/* if (clean_quit) {
		/ * try to disconnect cleanly * /
		pc = camel_pop3_engine_command_new (
			pop3_engine, 0, NULL, NULL,
			cancellable, NULL, "QUIT\r\n");
		while (camel_pop3_engine_iterate (pop3_engine, NULL, cancellable, NULL) > 0)
			;
		camel_pop3_engine_command_free (pop3_engine, pc);
	}*/

exception:
	g_clear_object (&stream);
	g_clear_object (&pop3_engine);

	success = FALSE;

exit:
	g_free (host);

	g_mutex_lock (&store->priv->property_lock);
	if (pop3_engine != NULL)
		store->priv->engine = g_object_ref (pop3_engine);
	g_mutex_unlock (&store->priv->property_lock);

	g_clear_object (&pop3_engine);

	return success;
}