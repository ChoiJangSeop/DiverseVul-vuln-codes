send_display_mode (GdmSession *self,
                   GdmSessionConversation *conversation)
{
        GdmSessionDisplayMode mode;

        mode = gdm_session_get_display_mode (self);
        gdm_dbus_worker_call_set_session_display_mode (conversation->worker_proxy,
                                                       gdm_session_display_mode_to_string (mode),
                                                       NULL, NULL, NULL);
}