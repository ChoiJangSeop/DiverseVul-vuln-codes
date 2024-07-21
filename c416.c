gs_manager_finalize (GObject *object)
{
        GSManager *manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GS_IS_MANAGER (object));

        manager = GS_MANAGER (object);

        g_return_if_fail (manager->priv != NULL);

        if (manager->priv->bg_notify_id != 0) {
                gconf_client_remove_dir (manager->priv->client,
                                         GNOME_BG_KEY_DIR,
                                         NULL);
                gconf_client_notify_remove (manager->priv->client,
                                            manager->priv->bg_notify_id);
                manager->priv->bg_notify_id = 0;
        }
        if (manager->priv->bg != NULL) {
                g_object_unref (manager->priv->bg);
        }
        if (manager->priv->client != NULL) {
                g_object_unref (manager->priv->client);
        }

        free_themes (manager);
        g_free (manager->priv->logout_command);
        g_free (manager->priv->keyboard_command);
        g_free (manager->priv->away_message);

        remove_unfade_idle (manager);
        remove_timers (manager);

        gs_grab_release (manager->priv->grab);

        manager_stop_jobs (manager);

        gs_manager_destroy_windows (manager);

        manager->priv->active = FALSE;
        manager->priv->activate_time = 0;
        manager->priv->lock_enabled = FALSE;

        g_object_unref (manager->priv->fade);
        g_object_unref (manager->priv->grab);
        g_object_unref (manager->priv->theme_manager);

        G_OBJECT_CLASS (gs_manager_parent_class)->finalize (object);
}