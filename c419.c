gs_manager_destroy_windows (GSManager *manager)
{
        GdkDisplay  *display;
        GSList      *l;
        int          n_screens;
        int          i;

        g_return_if_fail (manager != NULL);
        g_return_if_fail (GS_IS_MANAGER (manager));

        if (manager->priv->windows == NULL) {
                return;
        }

        display = gdk_display_get_default ();

        n_screens = gdk_display_get_n_screens (display);

        for (i = 0; i < n_screens; i++) {
                g_signal_handlers_disconnect_by_func (gdk_display_get_screen (display, i),
                                                      on_screen_monitors_changed,
                                                      manager);
        }

        for (l = manager->priv->windows; l; l = l->next) {
                gs_window_destroy (l->data);
        }
        g_slist_free (manager->priv->windows);
        manager->priv->windows = NULL;
}