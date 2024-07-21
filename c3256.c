free_conversation (GdmSessionConversation *conversation)
{
        if (conversation->job != NULL) {
                g_warning ("Freeing conversation '%s' with active job", conversation->service_name);
        }

        g_free (conversation->service_name);
        g_free (conversation->starting_username);
        g_free (conversation->session_id);
        g_clear_object (&conversation->worker_manager_interface);

        if (conversation->worker_proxy != NULL) {
                g_signal_handlers_disconnect_by_func (conversation->worker_proxy,
                                                      G_CALLBACK (worker_on_username_changed),
                                                      conversation);
                g_signal_handlers_disconnect_by_func (conversation->worker_proxy,
                                                      G_CALLBACK (worker_on_session_exited),
                                                      conversation);
                g_signal_handlers_disconnect_by_func (conversation->worker_proxy,
                                                      G_CALLBACK (worker_on_reauthenticated),
                                                      conversation);
                g_signal_handlers_disconnect_by_func (conversation->worker_proxy,
                                                      G_CALLBACK (worker_on_saved_language_name_read),
                                                      conversation);
                g_signal_handlers_disconnect_by_func (conversation->worker_proxy,
                                                      G_CALLBACK (worker_on_saved_session_name_read),
                                                      conversation);
                g_signal_handlers_disconnect_by_func (conversation->worker_proxy,
                                                      G_CALLBACK (worker_on_cancel_pending_query),
                                                      conversation);
                g_clear_object (&conversation->worker_proxy);
        }
        g_clear_object (&conversation->session);
        g_free (conversation);
}