mail_config_ews_autodiscover_run (EMailConfigEwsAutodiscover *autodiscover)
{
	EActivity *activity;
	EMailConfigServicePage *page;
	EMailConfigServiceBackend *backend;
	CamelSettings *settings;
	ESource *source;
	GCancellable *cancellable;
	AsyncContext *async_context;
	GTask *task;

	backend = e_mail_config_ews_autodiscover_get_backend (autodiscover);
	page = e_mail_config_service_backend_get_page (backend);
	source = e_mail_config_service_backend_get_source (backend);
	settings = e_mail_config_service_backend_get_settings (backend);

	if (!e_source_has_extension (source, E_SOURCE_EXTENSION_AUTHENTICATION)) {
		ESource *collection;

		collection = e_mail_config_service_backend_get_collection (backend);
		if (collection && e_source_has_extension (collection, E_SOURCE_EXTENSION_AUTHENTICATION)) {
			source = collection;
		}
	}

	activity = e_mail_config_activity_page_new_activity (E_MAIL_CONFIG_ACTIVITY_PAGE (page));
	cancellable = e_activity_get_cancellable (activity);

	e_activity_set_text (activity, _("Querying Autodiscover service"));

	gtk_widget_set_sensitive (GTK_WIDGET (autodiscover), FALSE);

	async_context = g_slice_new0 (AsyncContext);
	async_context->autodiscover = g_object_ref (autodiscover);
	async_context->activity = activity;  /* takes ownership */
	async_context->source = g_object_ref (source);
	async_context->ews_settings = g_object_ref (settings);
	async_context->email_address = g_strdup (e_mail_config_service_page_get_email_address (page));

	/*
	 * The GTask will be run in a new thread, which will invoke
	 * camel_ews_settings_set_{oaburl,hosturl}(), emiting signals that
	 * are bound to GTK+ UI signals, causing GTK+ calls in this
	 * secondary thread and consequently a crash. To avoid this, let's stop
	 * the property changes notifications while we are not in the main thread.
	 */
	g_object_freeze_notify (G_OBJECT (settings));

	task = g_task_new (autodiscover, cancellable, mail_config_ews_autodiscover_run_cb, async_context);
	g_task_set_source_tag (task, mail_config_ews_autodiscover_finish);
	g_task_set_task_data (task, async_context, async_context_free);

	g_task_run_in_thread (task, mail_config_ews_autodiscover_run_thread);

	g_object_unref (task);
}