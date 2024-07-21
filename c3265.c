register_worker (GdmDBusWorkerManager  *worker_manager_interface,
                 GDBusMethodInvocation *invocation,
                 GdmSession            *self)
{
        GdmSessionConversation *conversation;
        GDBusConnection *connection;
        GList *connection_node;
        GCredentials *credentials;
        GPid pid;

        g_debug ("GdmSession: Authenticating new connection");

        connection = g_dbus_method_invocation_get_connection (invocation);
        connection_node = g_list_find (self->priv->pending_worker_connections, connection);

        if (connection_node == NULL) {
                g_debug ("GdmSession: Ignoring connection that we aren't tracking");
                return FALSE;
        }

        /* connection was ref'd when it was added to list, we're taking that
         * reference over and removing it from the list
         */
        self->priv->pending_worker_connections =
                g_list_delete_link (self->priv->pending_worker_connections,
                                    connection_node);

        g_signal_handlers_disconnect_by_func (connection,
                                              G_CALLBACK (on_worker_connection_closed),
                                              self);

        credentials = g_dbus_connection_get_peer_credentials (connection);
        pid = g_credentials_get_unix_pid (credentials, NULL);

        conversation = find_conversation_by_pid (self, (GPid) pid);

        if (conversation == NULL) {
                g_warning ("GdmSession: New worker connection is from unknown source");

                g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                                       G_DBUS_ERROR_ACCESS_DENIED,
                                                       "Connection is not from a known conversation");
                g_dbus_connection_close_sync (connection, NULL, NULL);
                return TRUE;
        }

        g_dbus_method_invocation_return_value (invocation, NULL);

        conversation->worker_proxy = gdm_dbus_worker_proxy_new_sync (connection,
                                                                     G_DBUS_PROXY_FLAGS_NONE,
                                                                     NULL,
                                                                     GDM_WORKER_DBUS_PATH,
                                                                     NULL, NULL);
        /* drop the reference we stole from the pending connections list
         * since the proxy owns the connection now */
        g_object_unref (connection);

        g_dbus_proxy_set_default_timeout (G_DBUS_PROXY (conversation->worker_proxy), G_MAXINT);

        g_signal_connect (conversation->worker_proxy,
                          "username-changed",
                          G_CALLBACK (worker_on_username_changed), conversation);
        g_signal_connect (conversation->worker_proxy,
                          "session-exited",
                          G_CALLBACK (worker_on_session_exited), conversation);
        g_signal_connect (conversation->worker_proxy,
                          "reauthenticated",
                          G_CALLBACK (worker_on_reauthenticated), conversation);
        g_signal_connect (conversation->worker_proxy,
                          "saved-language-name-read",
                          G_CALLBACK (worker_on_saved_language_name_read), conversation);
        g_signal_connect (conversation->worker_proxy,
                          "saved-session-name-read",
                          G_CALLBACK (worker_on_saved_session_name_read), conversation);
        g_signal_connect (conversation->worker_proxy,
                          "cancel-pending-query",
                          G_CALLBACK (worker_on_cancel_pending_query), conversation);

        conversation->worker_manager_interface = g_object_ref (worker_manager_interface);
        g_debug ("GdmSession: worker connection is %p", connection);

        g_debug ("GdmSession: Emitting conversation-started signal");
        g_signal_emit (self, signals[CONVERSATION_STARTED], 0, conversation->service_name);

        if (self->priv->user_verifier_interface != NULL) {
                gdm_dbus_user_verifier_emit_conversation_started (self->priv->user_verifier_interface,
                                                                  conversation->service_name);
        }

        if (conversation->starting_invocation != NULL) {
                if (conversation->starting_username != NULL) {
                        gdm_session_setup_for_user (self, conversation->service_name, conversation->starting_username);

                        g_clear_pointer (&conversation->starting_username,
                                         (GDestroyNotify)
                                         g_free);
                } else {
                        gdm_session_setup (self, conversation->service_name);
                }
        }

        g_debug ("GdmSession: Conversation started");

        return TRUE;
}