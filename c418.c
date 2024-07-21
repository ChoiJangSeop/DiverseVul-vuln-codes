gs_manager_create_windows_for_screen (GSManager *manager,
                                      GdkScreen *screen)
{
        GSWindow *window;
        int       n_monitors;
        int       i;

        g_return_if_fail (manager != NULL);
        g_return_if_fail (GS_IS_MANAGER (manager));
        g_return_if_fail (GDK_IS_SCREEN (screen));

        g_object_ref (manager);
        g_object_ref (screen);

        n_monitors = gdk_screen_get_n_monitors (screen);

        gs_debug ("Creating %d windows for screen %d", n_monitors, gdk_screen_get_number (screen));

        for (i = 0; i < n_monitors; i++) {
                window = gs_window_new (screen, i, manager->priv->lock_active);

                gs_window_set_user_switch_enabled (window, manager->priv->user_switch_enabled);
                gs_window_set_logout_enabled (window, manager->priv->logout_enabled);
                gs_window_set_logout_timeout (window, manager->priv->logout_timeout);
                gs_window_set_logout_command (window, manager->priv->logout_command);
                gs_window_set_keyboard_enabled (window, manager->priv->keyboard_enabled);
                gs_window_set_keyboard_command (window, manager->priv->keyboard_command);
                gs_window_set_away_message (window, manager->priv->away_message);

                connect_window_signals (manager, window);

                manager->priv->windows = g_slist_append (manager->priv->windows, window);
        }

        g_object_unref (screen);
        g_object_unref (manager);
}