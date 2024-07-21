gdm_session_start_reauthentication (GdmSession *session,
                                    GPid        pid_of_caller,
                                    uid_t       uid_of_caller)
{
        GdmSessionConversation *conversation = session->priv->session_conversation;

        g_return_if_fail (conversation != NULL);

        conversation->reauth_pid_of_caller = pid_of_caller;

        gdm_dbus_worker_call_start_reauthentication (conversation->worker_proxy,
                                                     (int) pid_of_caller,
                                                     (int) uid_of_caller,
                                                     NULL,
                                                     (GAsyncReadyCallback) on_reauthentication_started_cb,
                                                     conversation);
}