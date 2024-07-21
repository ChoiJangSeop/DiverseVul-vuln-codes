on_screen_monitors_changed (GdkScreen *screen,
                            GSManager *manager)
{
        GSList *l;
        int     n_monitors;
        int     n_windows;
        int     i;

        n_monitors = gdk_screen_get_n_monitors (screen);
        n_windows = g_slist_length (manager->priv->windows);

        gs_debug ("Monitors changed for screen %d: num=%d",
                  gdk_screen_get_number (screen),
                  n_monitors);

        if (n_monitors > n_windows) {
                /* add more windows */
                for (i = n_windows; i < n_monitors; i++) {
                        gs_manager_create_window_for_monitor (manager, screen, i);
                }
        } else {
                /* remove the extra windows */
                l = manager->priv->windows;
                while (l != NULL) {
                        GdkScreen *this_screen;
                        int        this_monitor;
                        GSList    *next = l->next;

                        this_screen = gs_window_get_screen (GS_WINDOW (l->data));
                        this_monitor = gs_window_get_monitor (GS_WINDOW (l->data));
                        if (this_screen == screen && this_monitor >= n_monitors) {
                                manager_maybe_stop_job_for_window (manager, GS_WINDOW (l->data));
                                g_hash_table_remove (manager->priv->jobs, l->data);
                                gs_window_destroy (GS_WINDOW (l->data));
                                manager->priv->windows = g_slist_delete_link (manager->priv->windows, l);
                        }
                        l = next;
                }
        }
}