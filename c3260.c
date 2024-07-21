gdm_session_select_session (GdmSession *self,
                            const char *text)
{
        GHashTableIter iter;
        gpointer key, value;

        g_debug ("GdmSession: selecting session '%s'", text);

        g_free (self->priv->selected_session);
        self->priv->selected_session = g_strdup (text);

        update_session_type (self);

        g_hash_table_iter_init (&iter, self->priv->conversations);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                GdmSessionConversation *conversation;

                conversation = (GdmSessionConversation *) value;

                gdm_dbus_worker_call_set_session_name (conversation->worker_proxy,
                                                       get_session_name (self),
                                                       NULL, NULL, NULL);
        }
}