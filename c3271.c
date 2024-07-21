LOCAL char *value_for_key_as_string(MMDB_entry_s *start, char *key)
{
    MMDB_entry_data_s entry_data;
    const char *path[] = { key, NULL };
    MMDB_aget_value(start, &entry_data, path);
    return mmdb_strndup((char *)entry_data.utf8_string, entry_data.data_size);
}