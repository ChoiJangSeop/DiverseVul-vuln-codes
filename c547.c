_save_user_settings (GdmSessionWorker *worker,
                     const char       *home_dir)
{
        GError *error;

        if (!gdm_session_settings_is_loaded (worker->priv->user_settings)) {
                return;
        }

        error = NULL;
        if (!gdm_session_settings_save (worker->priv->user_settings,
                                        home_dir, &error)) {
                g_warning ("could not save session and language settings: %s",
                           error->message);
                g_error_free (error);
        }
}