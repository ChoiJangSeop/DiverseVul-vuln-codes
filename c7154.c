mail_config_ews_autodiscover_run_thread (GTask *task,
					 gpointer source_object,
					 gpointer task_data,
					 GCancellable *cancellable)
{
	AsyncContext *async_context = task_data;
	GError *local_error = NULL;
	gboolean success = FALSE;

	if (!g_cancellable_set_error_if_cancelled (cancellable, &local_error) && !local_error) {
		gboolean without_password;

		without_password = e_ews_connection_utils_get_without_password (async_context->ews_settings);
		if (without_password) {
			success = e_ews_autodiscover_ws_url_sync (async_context->source,
				async_context->ews_settings, async_context->email_address, "",
				cancellable, &local_error);
		}

		if (!without_password || g_error_matches (local_error, SOUP_HTTP_ERROR, SOUP_STATUS_UNAUTHORIZED)) {
			EShell *shell;

			e_ews_connection_utils_force_off_ntlm_auth_check ();
			g_clear_error (&local_error);

			shell = e_shell_get_default ();

			success = e_credentials_prompter_loop_prompt_sync (e_shell_get_credentials_prompter (shell),
				async_context->source, E_CREDENTIALS_PROMPTER_PROMPT_FLAG_ALLOW_SOURCE_SAVE,
				mail_config_ews_autodiscover_sync, async_context, cancellable, &local_error);
		}
	}

	if (local_error != NULL) {
		g_task_return_error (task, local_error);
	} else {
		g_task_return_boolean (task, success);
	}
}