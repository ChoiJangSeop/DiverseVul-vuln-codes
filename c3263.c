send_setup_for_program (GdmSession *self,
                        const char *service_name,
                        const char *username,
                        const char *log_file)
{
        const char     *display_name;
        const char     *display_device;
        const char     *display_seat_id;
        const char     *display_hostname;
        const char     *display_x11_authority_file;
        GdmSessionConversation *conversation;

        g_assert (service_name != NULL);

        if (self->priv->display_name != NULL) {
                display_name = self->priv->display_name;
        } else {
                display_name = "";
        }
        if (self->priv->display_hostname != NULL) {
                display_hostname = self->priv->display_hostname;
        } else {
                display_hostname = "";
        }
        if (self->priv->display_device != NULL) {
                display_device = self->priv->display_device;
        } else {
                display_device = "";
        }
        if (self->priv->display_seat_id != NULL) {
                display_seat_id = self->priv->display_seat_id;
        } else {
                display_seat_id = "";
        }
        if (self->priv->display_x11_authority_file != NULL) {
                display_x11_authority_file = self->priv->display_x11_authority_file;
        } else {
                display_x11_authority_file = "";
        }

        g_debug ("GdmSession: Beginning setup for session for program using PAM service %s", service_name);

        conversation = find_conversation_by_name (self, service_name);
        if (conversation != NULL) {
                gdm_dbus_worker_call_setup_for_program (conversation->worker_proxy,
                                                        service_name,
                                                        username,
                                                        display_name,
                                                        display_x11_authority_file,
                                                        display_device,
                                                        display_seat_id,
                                                        display_hostname,
                                                        self->priv->display_is_local,
                                                        self->priv->display_is_initial,
                                                        log_file,
                                                        NULL,
                                                        (GAsyncReadyCallback) on_setup_complete_cb,
                                                        conversation);
        }
}