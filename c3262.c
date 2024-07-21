send_session_type (GdmSession *self,
                   GdmSessionConversation *conversation)
{
        const char *session_type = "x11";

        if (self->priv->session_type != NULL) {
                session_type = self->priv->session_type;
        }

        gdm_dbus_worker_call_set_environment_variable (conversation->worker_proxy,
                                                       "XDG_SESSION_TYPE",
                                                       session_type,
                                                       NULL, NULL, NULL);
}