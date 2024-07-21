static void async_polkit_query_free(AsyncPolkitQuery *q) {

        if (!q)
                return;

        sd_bus_slot_unref(q->slot);

        if (q->registry && q->request)
                hashmap_remove(q->registry, q->request);

        sd_bus_message_unref(q->request);
        sd_bus_message_unref(q->reply);

        free(q->action);
        strv_free(q->details);

        free(q);
}