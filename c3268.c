send_setup_for_user (GdmSession *self,
                     const char *service_name)
{
        const char     *display_name;
        const char     *display_device;
        const char     *display_seat_id;
        const char     *display_hostname;
        const char     *display_x11_authority_file;
        const char     *selected_user;
        GdmSessionConversation *conversation;

        g_assert (service_name != NULL);

        conversation = find_conversation_by_name (self, service_name);

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
        if (self->priv->selected_user != NULL) {
                selected_user = self->priv->selected_user;
        } else {
                selected_user = "";
        }

        g_debug ("GdmSession: Beginning setup for user %s", self->priv->selected_user);

        if (conversation != NULL) {
                gdm_dbus_worker_call_setup_for_user (conversation->worker_proxy,
                                                     service_name,
                                                     selected_user,
                                                     display_name,
                                                     display_x11_authority_file,
                                                     display_device,
                                                     display_seat_id,
                                                     display_hostname,
                                                     self->priv->display_is_local,
                                                     self->priv->display_is_initial,
                                                     NULL,
                                                     (GAsyncReadyCallback) on_setup_complete_cb,
                                                     conversation);
        }
}