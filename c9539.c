gint vdagent_connection_get_peer_pid(VDAgentConnection *self,
                                     GError           **err)
{
    VDAgentConnectionPrivate *priv = vdagent_connection_get_instance_private(self);
    GSocket *sock;
    GCredentials *cred;
    gint pid = -1;

    g_return_val_if_fail(G_IS_SOCKET_CONNECTION(priv->io_stream), pid);

    sock = g_socket_connection_get_socket(G_SOCKET_CONNECTION(priv->io_stream));
    cred = g_socket_get_credentials(sock, err);
    if (cred) {
        pid = g_credentials_get_unix_pid(cred, err);
        g_object_unref(cred);
    }

    return pid;
}