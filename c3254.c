free_conversation (GdmSessionConversation *conversation)
{
        if (conversation->job != NULL) {
                g_warning ("Freeing conversation '%s' with active job", conversation->service_name);
        }

        g_free (conversation->service_name);
        g_free (conversation->starting_username);
        g_free (conversation->session_id);
        g_clear_object (&conversation->worker_manager_interface);
        g_clear_object (&conversation->worker_proxy);
        g_clear_object (&conversation->session);
        g_free (conversation);
}