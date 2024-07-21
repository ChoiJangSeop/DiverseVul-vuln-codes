read_one_cert (ReadFromGConfInfo *info,
               const char *setting_name,
               const char *key)
{
	char *value = NULL;

	if (!nm_gconf_get_string_helper (info->client, info->dir, key, setting_name, &value))
		return;

	g_object_set_data_full (G_OBJECT (info->connection),
	                        key, value,
	                        (GDestroyNotify) g_free);
}