ews_connection_constructed (GObject *object)
{
	EEwsConnection *cnc = E_EWS_CONNECTION (object);
	gint log_level;

	/* Chain up to parent's method. */
	G_OBJECT_CLASS (e_ews_connection_parent_class)->constructed (object);

	cnc->priv->soup_thread = g_thread_new (NULL, e_ews_soup_thread, cnc);

	cnc->priv->soup_session = soup_session_async_new_with_options (
		SOUP_SESSION_ASYNC_CONTEXT, cnc->priv->soup_context,
		NULL);

	/* Do not use G_BINDING_SYNC_CREATE because the property_lock is
	 * not initialized and we don't have a GProxyResolver yet anyway. */
	e_binding_bind_property (
		cnc, "proxy-resolver",
		cnc->priv->soup_session, "proxy-resolver",
		G_BINDING_DEFAULT);

	cnc->priv->version = E_EWS_EXCHANGE_UNKNOWN;

	log_level = e_ews_debug_get_log_level ();

	if (log_level >= 2) {
		SoupLogger *logger;
		logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);

		if (log_level >= 3) {
			soup_logger_set_printer (logger, e_ews_soup_log_printer, NULL, NULL);
			g_log_set_handler (
				G_LOG_DOMAIN,
				G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING |
				G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO,
				e_ews_debug_handler, cnc);
		}

		soup_session_add_feature (
			cnc->priv->soup_session,
			SOUP_SESSION_FEATURE (logger));
		g_object_unref (logger);
	}

	soup_session_add_feature_by_type (cnc->priv->soup_session,
					  SOUP_TYPE_COOKIE_JAR);

	g_signal_connect (
		cnc->priv->soup_session, "authenticate",
		G_CALLBACK (ews_connection_authenticate), cnc);

	e_ews_connection_utils_prepare_auth_method (cnc->priv->soup_session,
		camel_ews_settings_get_auth_mechanism (cnc->priv->settings));
}