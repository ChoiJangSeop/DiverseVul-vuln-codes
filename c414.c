gs_manager_create_window_for_monitor (GSManager *manager,
                                      GdkScreen *screen,
                                      int        monitor)
{
        GSWindow    *window;
        GdkRectangle rect;

        gdk_screen_get_monitor_geometry (screen, monitor, &rect);

        gs_debug ("Creating window for monitor %d [%d,%d] (%dx%d)",
                  monitor, rect.x, rect.y, rect.width, rect.height);

        window = gs_window_new (screen, monitor, manager->priv->lock_active);

        gs_window_set_user_switch_enabled (window, manager->priv->user_switch_enabled);
        gs_window_set_logout_enabled (window, manager->priv->logout_enabled);
        gs_window_set_logout_timeout (window, manager->priv->logout_timeout);
        gs_window_set_logout_command (window, manager->priv->logout_command);
        gs_window_set_keyboard_enabled (window, manager->priv->keyboard_enabled);
        gs_window_set_keyboard_command (window, manager->priv->keyboard_command);
        gs_window_set_status_message (window, manager->priv->status_message);

        connect_window_signals (manager, window);

        manager->priv->windows = g_slist_append (manager->priv->windows, window);
}