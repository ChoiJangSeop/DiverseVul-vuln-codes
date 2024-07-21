listener_ref_entry_has_connection (gpointer key,
                                   gpointer value,
                                   gpointer user_data)
{
        GSListenerRefEntry *entry;
        const char         *connection;
        gboolean            matches;

        entry = (GSListenerRefEntry *)value;
        connection = (const char *) user_data;

        matches = FALSE;
        if (connection != NULL && entry->connection != NULL) {
                matches = (strcmp (connection, entry->connection) == 0);
                if (matches) {
                        gs_debug ("removing %s from %s for reason '%s' on connection %s",
                                  get_name_for_entry_type (entry->entry_type),
                                  entry->application,
                                  entry->reason,
                                  entry->connection);
                }
        }

        return matches;
}