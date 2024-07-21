send_environment_variable (const char             *key,
                           const char             *value,
                           GdmSessionConversation *conversation)
{
        gdm_dbus_worker_call_set_environment_variable (conversation->worker_proxy,
                                                       key, value,
                                                       NULL, NULL, NULL);
}