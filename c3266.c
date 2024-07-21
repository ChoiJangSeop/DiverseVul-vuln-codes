gdm_session_open_session (GdmSession *self,
                          const char *service_name)
{
        GdmSessionConversation *conversation;

        g_return_if_fail (GDM_IS_SESSION (self));

        conversation = find_conversation_by_name (self, service_name);

        if (conversation != NULL) {
                send_display_mode (self, conversation);
                send_session_type (self, conversation);

                gdm_dbus_worker_call_open (conversation->worker_proxy,
                                           NULL,
                                           (GAsyncReadyCallback) on_opened, conversation);
        }
}