gdm_session_worker_finalize (GObject *object)
{
        GdmSessionWorker *worker;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_SESSION_WORKER (object));

        worker = GDM_SESSION_WORKER (object);

        g_return_if_fail (worker->priv != NULL);

        gdm_session_worker_unwatch_child (worker);

        if (worker->priv->child_pid > 0) {
                gdm_signal_pid (worker->priv->child_pid, SIGTERM);
                gdm_wait_on_pid (worker->priv->child_pid);
        }

        if (worker->priv->pam_handle != NULL) {
                gdm_session_worker_uninitialize_pam (worker, PAM_SUCCESS);
        }

        g_object_unref (worker->priv->user_settings);
        g_free (worker->priv->service);
        g_free (worker->priv->x11_display_name);
        g_free (worker->priv->x11_authority_file);
        g_free (worker->priv->display_device);
        g_free (worker->priv->display_seat_id);
        g_free (worker->priv->hostname);
        g_free (worker->priv->username);
        g_free (worker->priv->server_address);
        g_strfreev (worker->priv->arguments);
        g_strfreev (worker->priv->extensions);

        g_hash_table_unref (worker->priv->reauthentication_requests);

        G_OBJECT_CLASS (gdm_session_worker_parent_class)->finalize (object);
}