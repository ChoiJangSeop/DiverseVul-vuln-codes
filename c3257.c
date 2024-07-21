gdm_session_authenticate (GdmSession *self,
                          const char *service_name)
{
        GdmSessionConversation *conversation;

        g_return_if_fail (GDM_IS_SESSION (self));

        conversation = find_conversation_by_name (self, service_name);
        if (conversation != NULL) {
                gdm_dbus_worker_call_authenticate (conversation->worker_proxy,
                                                   NULL,
                                                   (GAsyncReadyCallback) on_authenticate_cb,
                                                   conversation);
        }
}