greeter_display_started (GdmManager *manager,
                         GdmDisplay *display)
{
        if (manager->priv->ran_once) {
                return;
        }

        maybe_start_pending_initial_login (manager, display);

        manager->priv->ran_once = TRUE;
}