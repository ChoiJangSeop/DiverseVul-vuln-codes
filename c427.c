gs_window_destroy (GSWindow *window)
{
        g_return_if_fail (GS_IS_WINDOW (window));

        if (window->priv->lock_pid > 0) {
                gs_window_dialog_finish (window);
        }

        remove_popup_dialog_idle (window);
        remove_command_watches (window);
        remove_watchdog_timer (window);

        if (window->priv->lock_box != NULL) {
                gtk_container_remove (GTK_CONTAINER (window->priv->vbox), GTK_WIDGET (window->priv->lock_box));
                window->priv->lock_box = NULL;

                g_signal_emit (window, signals [DIALOG_DOWN], 0);
        }

        gtk_widget_destroy (GTK_WIDGET (window));
}