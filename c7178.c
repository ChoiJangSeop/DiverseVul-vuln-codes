mail_config_ews_autodiscover_run_cb (GObject *source_object,
                                     GAsyncResult *result,
                                     gpointer user_data)
{
	AsyncContext *async_context = user_data;
	EMailConfigEwsAutodiscover *autodiscover;
	EAlertSink *alert_sink;
	GError *error = NULL;
	EMailConfigServiceBackend *backend;
	CamelSettings *settings;

	autodiscover = async_context->autodiscover;
	alert_sink = e_activity_get_alert_sink (async_context->activity);

	mail_config_ews_autodiscover_finish (E_MAIL_CONFIG_EWS_AUTODISCOVER (source_object), result, &error);

	backend = e_mail_config_ews_autodiscover_get_backend (autodiscover);
	settings = e_mail_config_service_backend_get_settings (backend);
	/*
	 * And unstop since we are back to the main thread.
	 */
	g_object_thaw_notify (G_OBJECT (settings));

	if (e_activity_handle_cancellation (async_context->activity, error)) {
		g_error_free (error);

	} else if (error != NULL) {
		e_alert_submit (
			alert_sink,
			"ews:autodiscovery-error",
			error->message, NULL);
		g_error_free (error);
	}

	gtk_widget_set_sensitive (GTK_WIDGET (autodiscover), TRUE);
}