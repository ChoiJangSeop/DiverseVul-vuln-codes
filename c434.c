listener_ref_entry_remove_for_connection (GSListener  *listener,
                                          int          entry_type,
                                          const char  *connection)
{
        gboolean    removed;
        guint       n_removed;
        GHashTable *hash;

        hash = get_hash_for_entry_type (listener, entry_type);

        removed = FALSE;
        n_removed = g_hash_table_foreach_remove (hash, listener_ref_entry_has_connection, (gpointer)connection);

        removed = (n_removed > 0);

        return removed;
}